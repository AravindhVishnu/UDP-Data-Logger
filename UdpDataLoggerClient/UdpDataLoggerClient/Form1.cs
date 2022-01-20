﻿using System;
using System.IO;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Net;
using System.Net.Sockets;
using System.Timers;
using System.Diagnostics;

namespace UdpDataLoggerClient
{
    // UDP data logger client used to receive data from an embedded device.
    // After the logging is finished, the data is stored in a CSV file.
    // A UDP server has also been implemented in the same program for the purpose
    // of testing the client. The data the client expects to receive can easily
    // be modified. As an example, the server sends out three phase voltage waveforms.
    // Some GUI functionality is implemented for ease of use.
    public partial class Form1 : Form
    {
        private Socket _socket;

        private System.Timers.Timer _timer;

        private bool _udp_com_running;

        private float[] _values;
        private byte[] _data;

        private int _totalBytes;

        private IPEndPoint client_end_point;
        private EndPoint client_ep;

        private IPEndPoint server_end_point;
        private EndPoint server_ep;

        private MemoryStream _udp_data_log_stream;
        private StreamWriter _udp_data_log_stream_writer;

        IFormatProvider _iFormatProvider;

        private bool _client;

        private bool _connectFailed;

        private bool _initTransfer;
        private string _header;

        private float _prevValue;
        private uint _tCnt;
        private float _simulCnt;

        // Set this to the sample time with which the embedded device sends out data.
        // As an example (using the built-in server), the sample time is set to 1ms.
        // This value is also used to detect lost datagrams.
        private const float DELTA_T = 0.001f;

        // Delegates needed for the InvokeRequired code pattern which is useful in
        // multi threaded event driven GUI code.
        public delegate void addToListBox(string paramString);
        public delegate void clearListBox();
        public delegate void addToLabel7(int color);
        public delegate void addToLabel8(string paramString);
        public delegate void generateStopButtonEvent();
        public delegate void setDefaultSettingsEvent();

        public Form1()
        {
            // Autogenerated code for the design of the GUI
            InitializeComponent();
        }

        public void DoAddToListBox(string paramString)
        {
            if (this.InvokeRequired)
            {
                addToListBox delegateMethod = new addToListBox(this.DoAddToListBox);
                this.Invoke(delegateMethod, new object[] { paramString });
            }
            else
            {
                this.listBox1.Items.Add(paramString);
                this.listBox1.TopIndex = this.listBox1.Items.Count - 1;
            }
        }

        public void DoClearListBox()
        {
            if (this.InvokeRequired)
            {
                clearListBox delegateMethod = new clearListBox(this.DoClearListBox);
                this.Invoke(delegateMethod);
            }
            else
            {
                this.listBox1.Items.Clear();
                this.listBox1.TopIndex = this.listBox1.Items.Count - 1;
            }
        }

        public void DoLabel7Add(int color)
        {
            if (this.InvokeRequired)
            {
                addToLabel7 delegateMethod = new addToLabel7(this.DoLabel7Add);
                this.Invoke(delegateMethod, new object[] { color });
            }
            else
            {
                // Set communication status
                if (color == 0)
                {
                    this.label7.ForeColor = System.Drawing.Color.Black;
                    this.label7.Text = "Stopped";
                }
                else if (color == 1)
                {
                    this.label7.ForeColor = System.Drawing.Color.Orange;
                    this.label7.Text = "Started";
                }
                else if (color == 2)
                {
                    this.label7.ForeColor = System.Drawing.Color.Green;
                    if (_client)
                    {
                        this.label7.Text = "Receiving Data";
                    }
                    else  // Server
                    {
                        this.label7.Text = "Sending Data";
                    }
                }
                else
                {
                    this.label7.ForeColor = System.Drawing.Color.Red;
                    this.label7.Text = "Error";
                }
            }
        }

        public void DoLabel8Add(string paramString)
        {
            if (this.InvokeRequired)
            {
                addToLabel8 delegateMethod = new addToLabel8(this.DoLabel8Add);
                this.Invoke(delegateMethod, new object[] { paramString });
            }
            else
            {
                this.label8.Text = paramString;
            }
        }

        public void DoGenerateStopButtonEvent()
        {
            if (this.InvokeRequired)
            {
                generateStopButtonEvent delegateMethod = new generateStopButtonEvent(this.DoGenerateStopButtonEvent);
                this.Invoke(delegateMethod, new object[] { });
            }
            else
            {
                // Generate a stop button click event
                this.button5.PerformClick();
            }
        }

        public void DoSetDefaultSettingsEvent()
        {
            if (this.InvokeRequired)
            {
                setDefaultSettingsEvent delegateMethod = new setDefaultSettingsEvent(this.DoSetDefaultSettingsEvent);
                this.Invoke(delegateMethod, new object[] { });
            }
            else
            {
                // Set default user parameters
                this.textBox1.Text = "localhost";
                this.textBox2.Text = "52000";
                this.textBox3.Text = "localhost";
                this.textBox4.Text = "52001";
                if (_client == false)  // Server
                {
                    // Generate a button1 click event
                    this.button1.PerformClick();
                    _client = true;
                }
            }
        }

        private void FORM1_CLOSING(object sender, FormClosingEventArgs e)
        {
            // Intercepting application exit
            if (_udp_com_running == true)
            {
                // Generate a stop button click event
                this.DoGenerateStopButtonEvent();
            }

            // Store user parameters in the Settings.settings database
            Properties.Settings.Default.clientIpAddress = this.textBox1.Text;
            Properties.Settings.Default.clientPortNumber = this.textBox2.Text;
            Properties.Settings.Default.serverIpAddress = this.textBox3.Text;
            Properties.Settings.Default.serverPortNumber = this.textBox4.Text;
            Properties.Settings.Default.clientEnabled = _client;
            Properties.Settings.Default.Save();

            // Store the program exit time in the event log file
            DateTime time_now = DateTime.Now;
            string time_now_text = time_now.ToString();
            string text = "Program exit at: " + time_now_text;
            Trace.WriteLine(text, "UDP Data Logger");
            Trace.Flush();
            Trace.Close();

            // Clear list box
            this.DoClearListBox();

            // Release resources
            if (_udp_com_running == true)
            {
                _socket.Close();
            }
            if (_socket != null)
            {
                _socket.Dispose();
            }
            _udp_data_log_stream_writer.Dispose();
            _udp_data_log_stream.Dispose();
            _timer.Dispose();
        }

        private void FORM1_LOAD(object sender, EventArgs e)
        {
            // Store the program start time in the event log file
            DateTime time_now = DateTime.Now;
            string time_now_text = time_now.ToString();
            string text = "Program start at: " + time_now_text;
            Trace.WriteLine(text, "UDP Data Logger");

            // Reduce/prevent flicker when the list box is updated
            this.DoubleBuffered = true;

            _values = new float[] { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
            _data = new byte[_values.Length * sizeof(float)];
            for (int i = 0; i < _data.Length; i++)
            {
                _data[i] = 0;
            }

            _prevValue = 0.0f;
            _totalBytes = 0;

            _tCnt = 0;
            _simulCnt = 0;

            // Read user parameter values from Settings.settings database
            _client = Properties.Settings.Default.clientEnabled;
            if (_client == true)
            {
                this.button1.Font = new Font(button1.Font, FontStyle.Bold);
                this.button1.ForeColor = System.Drawing.Color.DarkBlue;
                this.button2.Font = new Font(button1.Font, FontStyle.Regular);
                this.button2.ForeColor = System.Drawing.Color.Black;
                this.button4.Text = "Start Receiving";
                this.button5.Text = "Stop Receiving";
            }
            else  // Server
            {
                this.button1.Font = new Font(button1.Font, FontStyle.Regular);
                this.button1.ForeColor = System.Drawing.Color.Black;
                this.button2.Font = new Font(button1.Font, FontStyle.Bold);
                this.button2.ForeColor = System.Drawing.Color.DarkBlue;
                this.button4.Text = "Start Sending";
                this.button5.Text = "Stop Sending";
            }
            this.textBox1.Text = Properties.Settings.Default.clientIpAddress;
            this.textBox2.Text = Properties.Settings.Default.clientPortNumber;
            this.textBox3.Text = Properties.Settings.Default.serverIpAddress;
            this.textBox4.Text = Properties.Settings.Default.serverPortNumber;

            this.label7.Text = "Stopped";
            this.label7.ForeColor = System.Drawing.Color.Black;
            this.label8.Text = "0";

            _udp_com_running = false;
            _connectFailed = false;

            _initTransfer = true;
            _header = "t,uA,uB,uC,uAB,uBC,uCA";  // Header for the CSV file

            _udp_data_log_stream = new MemoryStream(0);
            _udp_data_log_stream_writer = new StreamWriter(_udp_data_log_stream);

            // Timer used to generate an event once every second to measure
            // total number of bytes transferred per second
            _timer = new System.Timers.Timer(1000);
            _timer.Elapsed += onTimerEvent;
            _timer.Stop();

            _iFormatProvider = new System.Globalization.CultureInfo("en-US");
        }

        private void onTimerEvent(Object source, ElapsedEventArgs e)
        {
            // This callback unction will be called in a separate thread at
            // every timer event (once every 1s)
            this.DoLabel8Add(_totalBytes.ToString());
            _totalBytes = 0;

            string textMessage = String.Format(_iFormatProvider, "Timer event raised at {0:HH:mm:ss.fff}", e.SignalTime);
            Console.WriteLine(textMessage);

            if (_client == false)  // Server
            {
                _udp_data_log_stream_writer.WriteLine(textMessage);
                _udp_data_log_stream_writer.Flush();
            }
        }

        private void exit_click(object sender, EventArgs e)
        {
            // FORM1_CLOSING will be called next
            Application.Exit();
        }

        private void setDefaultSettings_click(object sender, EventArgs e)
        {
            if (_udp_com_running == false)
            {
                // Set default settings
                this.DoSetDefaultSettingsEvent();
            }
            else
            {
                MessageBox.Show("Stop the UDP/IP communication before setting default settings");
            }
        }

        private void dataLogFile_click(object sender, EventArgs e)
        {
            if (_udp_com_running == false)
            {
                // Try opening the csv file and if it does not exist,
                // then open the text file
                try
                {
                    Process.Start("C:\\Temp\\udp_client_data_log.csv");
                }
                catch
                {
                    // Ignore the catch
                }

                try
                {
                    Process.Start("C:\\Temp\\udp_server_data_log.txt");
                }
                catch (Exception ex)
                {
                    MessageBox.Show("Open File Error: " + ex.Message);
                }
            }
            else
            {
                MessageBox.Show("Stop the UDP/IP communication before opening the data log file");
            }
        }

        private void eventLogFile_click(object sender, EventArgs e)
        {
            if (_udp_com_running == false)
            {
                try
                {
                    // Do a flush operation before opening the event log file
                    Trace.Flush();
                    string path = Path.Combine(Environment.CurrentDirectory, "EventLog.log");
                    Process.Start(path);
                }
                catch (Exception ex)
                {
                    MessageBox.Show("Open File Error: " + ex.Message);
                }
            }
            else
            {
                MessageBox.Show("Stop the UDP/IP communication before opening the event log file");
            }
        }

        private void stop_button_Click(object sender, EventArgs e)
        {
            if ((_udp_com_running == true) || (_connectFailed == true))
            {
                _udp_com_running = false;
                _connectFailed = false;
                _initTransfer = true;
                _totalBytes = 0;
                this.resetValues();
                try
                {
                    if (_socket != null)
                    {
                        _socket.Close();
                    }
                }
                catch (Exception ex)
                {
                    Console.WriteLine("Connection Error: " + ex.Message);
                }

                _timer.Stop();
                this.DoLabel7Add(0);
                this.DoLabel8Add("0");

                DateTime time_now = DateTime.Now;
                string time_now_text = time_now.ToString();
                string text = "UDP/IP Communication stopped at: " + time_now_text;
                this.DoAddToListBox(text);
                Trace.WriteLine(text, "UDP Data Logger");
                Trace.Flush();

                if (_udp_data_log_stream.Length != 0)  // Check if the stream is not empty
                {
                    {
                        // Different type of files depending on if running in client mode or server mode.
                        // For client mode (receive), the data is stored in a csv file and for server mode
                        // (send), just the number of bytes sent is stored in a text file.
                        string path;
                        if (_client == true)
                        {
                            path = "C:\\Temp\\udp_client_data_log.csv";
                        }
                        else  // Server
                        {
                            path = "C:\\Temp\\udp_server_data_log.txt";
                        }

                        {
                            // Create a file stream
                            FileStream my_file = new FileStream(path, FileMode.Create, FileAccess.Write);

                            // Copy all data from MemoryStream to FileStream
                            _udp_data_log_stream.WriteTo(my_file);
                            my_file.Flush();
                            my_file.Close();
                            _udp_data_log_stream.SetLength(0);
                        }

                        if (_client == false)  // Server
                        {
                            // Add info in the end
                            StreamWriter my_file = new StreamWriter(path, true);
                            my_file.WriteLine();
                            my_file.WriteLine(text);
                            my_file.WriteLine("IP Address: " + textBox1.Text);
                            my_file.WriteLine("Port Number: " + textBox2.Text);
                            my_file.Flush();
                            my_file.Close();
                        }
                    }

                    this.DoAddToListBox("UDP/IP data log is stored in file");
                    this.DoAddToListBox("");
                }
                else
                {
                    Console.WriteLine("No data stored in the memory stream");
                }
            }
            else
            {
                MessageBox.Show("UDP/IP communication cannot be stopped since it has previously not been started");
            }
        }

        private void start_button_Click(object sender, EventArgs e)
        {
            if (_udp_com_running == false)
            {
                try
                {
                    // Store the UDP/IP coomunication start time
                    DateTime time_now = DateTime.Now;
                    string time_now_text = time_now.ToString();
                    string text = "UDP/IP Communication started at: " + time_now_text;
                    this.DoAddToListBox(text);
                    Trace.WriteLine(text, "UDP Data Logger");

                    // Create endpoints
                    string client_localhost;
                    IPAddress client_ip_address;
                    int client_port_number;

                    client_localhost = textBox1.Text.ToLower();
                    if (client_localhost.Equals("localhost"))
                    {
                        client_ip_address = IPAddress.Parse("127.0.0.1");
                    }
                    else
                    {
                        client_ip_address = IPAddress.Parse(textBox1.Text);
                    }
                    client_port_number = int.Parse(textBox2.Text);

                    client_end_point = new IPEndPoint(client_ip_address, client_port_number);
                    client_ep = (EndPoint)client_end_point;

                    string server_localhost;
                    IPAddress server_ip_address;
                    int server_port_number;

                    server_localhost = textBox1.Text.ToLower();
                    if (server_localhost.Equals("localhost"))
                    {
                        server_ip_address = IPAddress.Parse("127.0.0.1");
                    }
                    else
                    {
                        server_ip_address = IPAddress.Parse(textBox3.Text);
                    }
                    server_port_number = int.Parse(textBox4.Text);

                    server_end_point = new IPEndPoint(server_ip_address, server_port_number);
                    server_ep = (EndPoint)server_end_point;

                    // Initialize a new instance of the socket
                    _socket = new Socket(AddressFamily.InterNetwork, SocketType.Dgram, ProtocolType.Udp)
                    {
                        // Make sure that all Socket APIs are non-blocking
                        Blocking = false
                    };
                    _socket.SetSocketOption(SocketOptionLevel.IP, SocketOptionName.PacketInformation, true);

                    if (_client == false)  // Server
                    {
                        _socket.Bind(server_ep);
                        _socket.BeginSendTo(_data, 0, _data.Length, SocketFlags.None, client_ep, new AsyncCallback(OnSend), _socket);
                    }
                    else // Client
                    {
                        _socket.Bind(client_ep);
                        _socket.BeginReceiveFrom(_data, 0, _data.Length, SocketFlags.None, ref server_ep, new AsyncCallback(OnReceive), _socket);
                    }

                    this.DoLabel7Add(1);
                    this.DoLabel8Add("0");

                    _udp_com_running = true;
                }
                catch (Exception ex)
                {
                    this.DoLabel7Add(3);
                    this.DoLabel8Add("0");
                    _connectFailed = true;
                    Trace.WriteLine("Connection Error: " + ex.Message, "UDP Data Logger");
                    DoGenerateStopButtonEvent();
                    MessageBox.Show("Connection Error: " + ex.Message, "UDP Client/Server", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
            }
            else
            {
                MessageBox.Show("UDP/IP communication already started");
            }
        }

        private void OnSend(IAsyncResult ar)
        {
            // Asynchronous callback called in a separate thread when data has been sent
            try
            {
                _timer.Start();
                int bytes = _socket.EndSend(ar);
                _totalBytes += bytes;

                DateTime time_now = DateTime.Now;
                string time_now_text = time_now.ToString();
                string textMessage = String.Format(_iFormatProvider, "{0}, Sample time: {1} - Number of bytes sent: {2}", time_now_text, _simulCnt, bytes);

                // Generate simulated data to be sent
                this.generateValues();

                // Convert from float array to byte array
                Buffer.BlockCopy(_values, 0, _data, 0, _data.Length);

                string dataMessage;
                if (_initTransfer == true)
                {
                    dataMessage = _header;
                    _initTransfer = false;
                }
                else
                {
                    // Structure the data in CSV format with max three decimals
                    dataMessage = String.Format(_iFormatProvider, "{0:0.###},{1:0.###},{2:0.###},{3:0.###},{4:0.###},{5:0.###},{6:0.###}", _values[0], _values[1], _values[2], _values[3], _values[4], _values[5], _values[6]);
                }
                this.DoAddToListBox(dataMessage);

                _udp_data_log_stream_writer.WriteLine(textMessage);
                _udp_data_log_stream_writer.Flush();

                // Sleep for the specified time before sending data
                int sleep_time_ms = (int)Math.Round(DELTA_T * 1000.0f);
                Thread.Sleep(sleep_time_ms);

                if (_udp_com_running == true)
                {
                    _socket.BeginSendTo(_data, 0, _data.Length, SocketFlags.None, client_ep, new AsyncCallback(OnSend), _socket);
                    this.DoLabel7Add(2);
                }
            }
            catch (Exception e)
            {
                this.DoLabel7Add(3);
                this.DoLabel8Add("0");
                Trace.WriteLine("Connection Error: " + e.Message, "UDP Data Logger");
                MessageBox.Show("Connection Error: " + e.Message, "UDP Server", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void OnReceive(IAsyncResult ar)
        {
            // Asynchronous callback called in a separate thread when data has been received
            _timer.Start();
            int bytes = 0;
            try
            {
                bytes = _socket.EndReceive(ar);
                _totalBytes += bytes;
                Console.WriteLine("Number of bytes received: {0}", bytes);
            }
            catch (Exception e)
            {
                // Will enter here when the UDP/IP client communication is stopped
                Console.WriteLine("EndReceive caused an exception: " + e.Message);
                return;
            }

            try
            {
                DateTime time_now = DateTime.Now;
                string time_now_text = time_now.ToString();
                string textMessage = String.Format(_iFormatProvider, "{0}, Sample time: {1} - Number of bytes received: {2}", time_now_text, _simulCnt, bytes);
                Console.WriteLine(textMessage);

                // Convert from byte array to float array
                Buffer.BlockCopy(_data, 0, _values, 0, _data.Length);

                // Detect lost samples
                float diff = _values[0] - _prevValue;
                if (diff > (1.1f * DELTA_T))
                {
                    uint lostDatagrams = (uint)Math.Round(diff / DELTA_T);
                    textMessage = String.Format(_iFormatProvider, "{0} Lost datagrams: {1}", time_now_text, lostDatagrams);
                    Console.WriteLine(textMessage);
                    Trace.WriteLine(textMessage, "UDP Data Logger");
                }
                _prevValue = _values[0];  // Feedback

                string dataMessage;
                if (_initTransfer == true)
                {
                    dataMessage = _header;
                    _initTransfer = false;
                }
                else
                {
                    // Structure the data in CSV format with max three decimals
                    dataMessage = String.Format(_iFormatProvider, "{0:0.###},{1:0.###},{2:0.###},{3:0.###},{4:0.###},{5:0.###},{6:0.###}", _values[0], _values[1], _values[2], _values[3], _values[4], _values[5], _values[6]);
                }
                this.DoAddToListBox(dataMessage);

                _udp_data_log_stream_writer.WriteLine(dataMessage);
                _udp_data_log_stream_writer.Flush();

                if (_udp_com_running == true)
                {
                    // Only receive if the UDP/IP communication is running
                    _socket.BeginReceiveFrom(_data, 0, _data.Length, SocketFlags.None, ref server_ep, new AsyncCallback(OnReceive), _socket);
                }

                this.DoLabel7Add(2);
            }
            catch (Exception e)
            {
                this.DoLabel7Add(3);
                Trace.WriteLine("Connection Error: " + e.Message, "UDP Data Logger");
                MessageBox.Show("Connection Error: " + e.Message, "UDP Client", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void button1_Click(object sender, EventArgs e)
        {
            if (_udp_com_running == false)
            {
                this.resetValues();
                _client = true;
                _totalBytes = 0;
                _initTransfer = true;
                _connectFailed = false;
                this.DoLabel7Add(0);
                this.DoLabel8Add("0");
                this.button1.Font = new Font(button1.Font, FontStyle.Bold);
                button1.ForeColor = System.Drawing.Color.DarkBlue;
                this.button2.Font = new Font(button2.Font, FontStyle.Regular);
                button2.ForeColor = System.Drawing.Color.Black;
                button4.Text = "Start Receiving";
                button5.Text = "Stop Receiving";
                _udp_data_log_stream.SetLength(0);
                this.DoAddToListBox("Switching to Client mode");
            }
            else
            {
                MessageBox.Show("Stop the UDP/IP communication before switching mode");
            }
        }

        private void button2_Click(object sender, EventArgs e)
        {
            if (_udp_com_running == false)
            {
                this.resetValues();
                _client = false;
                _totalBytes = 0;
                _initTransfer = true;
                _connectFailed = false;
                this.DoLabel7Add(0);
                this.DoLabel8Add("0");
                this.button1.Font = new Font(button1.Font, FontStyle.Regular);
                button1.ForeColor = System.Drawing.Color.Black;
                this.button2.Font = new Font(button2.Font, FontStyle.Bold);
                button2.ForeColor = System.Drawing.Color.DarkBlue;
                button4.Text = "Start Sending";
                button5.Text = "Stop Sending";
                _udp_data_log_stream.SetLength(0);
                this.DoAddToListBox("Switching to Server mode");
            }
            else
            {
                MessageBox.Show("Stop the UDP/IP communication before switching mode");
            }
        }

        private void generateValues()
        {
            // Just as an example, generate simulation data which is three phase voltage waveforms
            const float U_NET = 230.0f;  // 230V
            const float F_NET = 10.0f;  // 10Hz
            const float M_2PI = 2.0f * (float)Math.PI;
            const float TWOPIDIV3 = (2.0f / 3.0f) * (float)Math.PI;
            const float FOURPIDIV3 = (4.0f / 3.0f) * (float)Math.PI;
            const float TWOPI_DELTA_T = 2.0f * (float)Math.PI * DELTA_T;
            uint PERIOD_COUNT = (uint)Math.Round(1.0f / (F_NET * DELTA_T));

            // update simulation time
            _simulCnt += DELTA_T;

            // reference angle for the three phase sine waves
            float phi_net = TWOPI_DELTA_T * F_NET * (float)_tCnt;

            // wrap the counter at every end of the period
            if ((++_tCnt) >= PERIOD_COUNT)
            {
                _tCnt = 0;
            }

            // phase a angle
            float phase_a_angle = phi_net;

            // limit phase a angle between 0 and 2*PI
            if (phase_a_angle > M_2PI)
            {
                phase_a_angle -= M_2PI;
            }
            else if (phase_a_angle < 0.0f)
            {
                phase_a_angle += M_2PI;
            }
            else
            {
                // no change in the value of _phase_a_angle
            }

            // phase b angle is shifted 2*PI/3 radians (120 degrees)
            float phase_b_angle = phi_net - TWOPIDIV3;

            // limit phase b angle between 0 and 2*PI
            if (phase_b_angle > M_2PI)
            {
                phase_b_angle -= M_2PI;
            }
            else if (phase_b_angle < 0.0f)
            {
                phase_b_angle += M_2PI;
            }
            else
            {
                // no change in the value of _phase_b_angle
            }

            // phase b angle is shifted 4*PI/3 radians (240 degrees)
            float phase_c_angle = phi_net - FOURPIDIV3;

            // keep phase c angle between 0 and 2*PI
            if (phase_c_angle > M_2PI)
            {
                phase_c_angle -= M_2PI;
            }
            else if (phase_c_angle < 0.0f)
            {
                phase_c_angle += M_2PI;
            }
            else
            {
                // no change in the value of _phase_c_angle
            }

            // generation of the three phase voltage sine waves
            float u_na = U_NET * (float)Math.Sin(phase_a_angle);
            float u_nb = U_NET * (float)Math.Sin(phase_b_angle);
            float u_nc = U_NET * (float)Math.Sin(phase_c_angle);

            // line (phase-to-phase) voltages
            float u_ab = u_na - u_nb;
            float u_bc = u_nb - u_nc;
            float u_ca = u_nc - u_na;

            // Copy the simulated data to the _values float array
            _values[0] = _simulCnt;
            _values[1] = u_na;
            _values[2] = u_nb;
            _values[3] = u_nc;
            _values[4] = u_ab;
            _values[5] = u_bc;
            _values[6] = u_ca;
        }

        private void resetValues()
        {
            // Reset all simulation data
            for (int i = 0; i < _data.Length; i++)
            {
                _data[i] = 0;
            }
            for (int i = 0; i < _values.Length; i++)
            {
                _values[i] = 0;
            }
            _tCnt = 0;
            _simulCnt = 0.0f;
            _prevValue = 0.0f;
        }
    }
}