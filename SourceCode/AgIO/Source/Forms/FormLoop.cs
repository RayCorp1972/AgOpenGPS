﻿using AgIO.Properties;
using System;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Net.Sockets;
using System.Text;
using System.Windows.Forms;

namespace AgIO
{
    public partial class FormLoop : Form
    {
        [System.Runtime.InteropServices.DllImport("User32.dll")]
        private static extern bool SetForegroundWindow(IntPtr handle);

        [System.Runtime.InteropServices.DllImport("User32.dll")]
        private static extern bool ShowWindow(IntPtr hWind, int nCmdShow);

        public FormLoop()
        {
            InitializeComponent();
        }

        public StringBuilder logNMEASentence = new StringBuilder();

        public bool isKeyboardOn = true;

        public bool isSendToSerial = false, isSendToUDP = false;

        public bool isGPSSentencesOn = false, isSendNMEAToUDP;

        public double secondsSinceStart, twoSecondTimer, tenSecondTimer;

        public string lastSentence;

        public bool isPluginUsed;

        public int packetSizeNTRIP;

        public bool isViewAdvanced = false;
        public bool isLogNMEA;

        //The base directory where Drive will be stored and fields and vehicles branch from
        public string baseDirectory;

        //current directory of Comm storage
        public string commDirectory, commFileName = "";

        //First run
        private void FormLoop_Load(object sender, EventArgs e)
        {
            if (Settings.Default.setF_workingDirectory == "Default")
                baseDirectory = Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments) + "\\AgOpenGPS\\";
            else baseDirectory = Settings.Default.setF_workingDirectory + "\\AgOpenGPS\\";

            //get the fields directory, if not exist, create
            commDirectory = baseDirectory + "AgIO\\";
            string dir = Path.GetDirectoryName(commDirectory);
            if (!string.IsNullOrEmpty(dir) && !Directory.Exists(dir)) { Directory.CreateDirectory(dir); }

            if (Settings.Default.setUDP_isOn)
            {
                LoadUDPNetwork();
            }
            else
            {
                btnUDP.BackColor = Color.Silver;
                lblIP.Text = "Off";
            }

            //small view
            this.Width = 430;

            LoadLoopback();

            isSendNMEAToUDP = Properties.Settings.Default.setUDP_isSendNMEAToUDP;
            isPluginUsed = Properties.Settings.Default.setUDP_isUsePluginApp;

            packetSizeNTRIP = Properties.Settings.Default.setNTRIP_packetSize;

            isSendToSerial = Settings.Default.setNTRIP_sendToSerial;
            isSendToUDP = Settings.Default.setNTRIP_sendToUDP;

            //lblMount.Text = Properties.Settings.Default.setNTRIP_mount;

            lblGPS1Comm.Text = "---";
            lblIMUComm.Text = "---";
            lblMod1Comm.Text = "---";
            lblMod2Comm.Text = "---";
            //lblMod3Comm.Text = "---";

            //set baud and port from last time run
            baudRateGPS = Settings.Default.setPort_baudRateGPS;
            portNameGPS = Settings.Default.setPort_portNameGPS;
            wasGPSConnectedLastRun = Settings.Default.setPort_wasGPSConnected;
            if (wasGPSConnectedLastRun)
            {
                OpenGPSPort();
                if (spGPS.IsOpen) lblGPS1Comm.Text = portNameGPS;
            }

            // set baud and port for rtcm from last time run
            baudRateRtcm = Settings.Default.setPort_baudRateRtcm;
            portNameRtcm = Settings.Default.setPort_portNameRtcm;
            wasRtcmConnectedLastRun = Settings.Default.setPort_wasRtcmConnected;

            if (wasRtcmConnectedLastRun)
            {
                OpenRtcmPort();
            }

            //Open IMU
            portNameIMU = Settings.Default.setPort_portNameIMU;
            wasIMUConnectedLastRun = Settings.Default.setPort_wasIMUConnected;
            if (wasIMUConnectedLastRun)
            {
                OpenIMUPort();
                if (spIMU.IsOpen) lblIMUComm.Text = portNameIMU;
            }


            //same for Module1 port
            portNameModule1 = Settings.Default.setPort_portNameModule1;
            wasModule1ConnectedLastRun = Settings.Default.setPort_wasModule1Connected;
            if (wasModule1ConnectedLastRun)
            {
                OpenModule1Port();
                if (spModule1.IsOpen) lblMod1Comm.Text = portNameModule1;
            }

            //same for Module2 port
            portNameModule2 = Settings.Default.setPort_portNameModule2;
            wasModule2ConnectedLastRun = Settings.Default.setPort_wasModule2Connected;
            if (wasModule2ConnectedLastRun)
            {
                OpenModule2Port();
                if (spModule2.IsOpen) lblMod2Comm.Text = portNameModule2;
            }

            //same for Module3 port
            portNameModule3 = Settings.Default.setPort_portNameModule3;
            wasModule3ConnectedLastRun = Settings.Default.setPort_wasModule3Connected;
            if (wasModule3ConnectedLastRun)
            {
                OpenModule3Port();
                //if (spModule3.IsOpen) lblMod3Comm.Text = portNameModule3;
            }

            ConfigureNTRIP();

            string[] ports = System.IO.Ports.SerialPort.GetPortNames();

            if (ports.Length == 0)
            {
                lblSerialPorts.Text = "None";
            }
            else
            {
                for (int i = 0; i < ports.Length; i++)
                {
                    lblSerialPorts.Text = ports[i] + "\r\n";
                }
            }

            lastSentence = Properties.Settings.Default.setGPS_lastSentence;

            oneSecondLoopTimer.Enabled = true;
            pictureBox1.Visible = true;
            pictureBox1.BringToFront();
            pictureBox1.Dock = DockStyle.Fill;
        }

        private void FormLoop_FormClosing(object sender, FormClosingEventArgs e)
        {
            if (loopBackSocket != null)
            {
                try
                {
                    loopBackSocket.Shutdown(SocketShutdown.Both);
                }
                finally { loopBackSocket.Close(); }
            }

            if (UDPSocket != null)
            {
                try
                {
                    UDPSocket.Shutdown(SocketShutdown.Both);
                }
                finally { UDPSocket.Close(); }
            }
        }

        StringBuilder sbRTCM = new StringBuilder();
        private void oneSecondLoopTimer_Tick(object sender, EventArgs e)
        {
            if (oneSecondLoopTimer.Interval > 1000)
            {
                Controls.Remove(pictureBox1);
                pictureBox1.Dispose();
                oneSecondLoopTimer.Interval = 1000;
                return;
            }

            secondsSinceStart = (DateTime.Now - Process.GetCurrentProcess().StartTime).TotalSeconds;

            DoTraffic();

            lblCurentLon.Text = longitude.ToString("N7");
            lblCurrentLat.Text = latitude.ToString("N7");

            //do all the NTRIP routines
            DoNTRIPSecondRoutine();

            //send a hello to modules
            SendUDPMessage(helloFromAgIO, epModule);

            //send back to Drive proof of life
            //every 3 seconds
            if ((secondsSinceStart - twoSecondTimer) > 2)
            {
                if (isLogNMEA)
                {
                    using (StreamWriter writer = new StreamWriter("zAgIO_log.txt", true))
                    {
                        writer.Write(logNMEASentence.ToString());
                    }
                    logNMEASentence.Clear();
                }


                twoSecondTimer = secondsSinceStart;

                if (traffic.helloFromMachine < 3) btnMachine.BackColor = Color.LightGreen;
                else btnMachine.BackColor = Color.Transparent;

                if (traffic.helloFromAutoSteer < 3) btnSteer.BackColor = Color.LightGreen;
                else btnSteer.BackColor = Color.Transparent;

                if (traffic.helloFromIMU < 3) btnIMU.BackColor = Color.LightGreen;
                else btnIMU.BackColor = Color.Transparent;

            }

            //every 10 seconds
            if ((secondsSinceStart - tenSecondTimer) > 10)
            {
                if (isViewAdvanced)
                {
                    lblTotalMessageCount.Text = rList.Count.ToString();

                    try
                    {
                        //add the uniques messages to all the new ones
                        foreach (var item in aList)
                        {
                            if (item > 999 && item < 4096)
                                rList.Add(item);
                        }

                        //sort and group using Linq
                        sbRTCM.Clear();

                        var g = rList.GroupBy(i => i)
                            .OrderBy(grp => grp.Key);
                        int count = 0;
                        aList.Clear();

                        //Create the text box of unique message numbers
                        foreach (var grp in g)
                        {
                            aList.Add(grp.Key);
                            sbRTCM.AppendLine(grp.Key + " - " + grp.Count());
                            count++;
                        }

                        rList?.Clear();

                        //too many messages or trash
                        if (count > 17)
                        {
                            aList?.Clear();
                            sbRTCM.Clear();
                            sbRTCM.Append("Reset..");
                        }

                        lblMessagesFound.Text = count.ToString();
                    }

                    catch
                    {
                        sbRTCM.Clear();
                        sbRTCM.Append("Error");
                    }
                }

                #region Serial update

                if (wasIMUConnectedLastRun)
                {
                    if (!spIMU.IsOpen)
                    {
                        byte[] imuClose = new byte[] { 0x80, 0x81, 0x7C, 0xD4, 2, 1, 0, 83 };

                        //tell AOG IMU is disconnected
                        SendToLoopBackMessageAOG(imuClose);
                        wasIMUConnectedLastRun = false;
                        lblIMUComm.Text = "---";
                    }
                }

                if (wasGPSConnectedLastRun)
                {
                    if (!spGPS.IsOpen)
                    {
                        wasGPSConnectedLastRun = false;
                        lblGPS1Comm.Text = "---";
                    }
                }

                if (wasModule1ConnectedLastRun)
                {
                    if (!spModule1.IsOpen)
                    {
                        wasModule1ConnectedLastRun = false;
                        lblMod1Comm.Text = "---";
                    }
                }

                if (wasModule2ConnectedLastRun)
                {
                    if (!spModule2.IsOpen)
                    {
                        wasModule2ConnectedLastRun = false;
                        lblMod2Comm.Text = "---";
                    }
                }

                if (wasModule3ConnectedLastRun)
                {
                    if (!spModule3.IsOpen)
                    {
                        wasModule3ConnectedLastRun = false;
                    }
                }

                #endregion

                tenSecondTimer = secondsSinceStart;

            }

            if (isViewAdvanced)
            {
                sbRTCM.Append(".");
                lblMessages.Text = sbRTCM.ToString();
            }

        }

        private void DoTraffic()
        {
            traffic.helloFromMachine++;
            traffic.helloFromAutoSteer++;
            traffic.helloFromIMU++;

            //lblToAOG.Text = traffic.cntrPGNToAOG == 0 ? "--" : (traffic.cntrPGNToAOG).ToString();
            //lblFromAOG.Text = traffic.cntrPGNFromAOG == 0 ? "--" : (traffic.cntrPGNFromAOG).ToString();

            lblFromGPS.Text = traffic.cntrGPSOut == 0 ? "--" : (traffic.cntrGPSOut).ToString();

            lblToSteer.Text = traffic.cntrSteerIn == 0 ? "--" : (traffic.cntrSteerIn).ToString();
            lblFromSteer.Text = traffic.cntrSteerOut == 0 ? "--" : (traffic.cntrSteerOut).ToString();

            lblToMachine.Text = traffic.cntrMachineIn == 0 ? "--" : (traffic.cntrMachineIn).ToString();
            lblFromMachine.Text = traffic.cntrMachineOut == 0 ? "--" : (traffic.cntrMachineOut).ToString();

            lblFromMU.Text = traffic.cntrIMUOut == 0 ? "--" : (traffic.cntrIMUOut).ToString();

            if (traffic.cntrGPSOut > 0) btnGPS.BackColor = Color.LightGreen;
            else btnGPS.BackColor = Color.Transparent;

            traffic.cntrPGNToAOG = traffic.cntrPGNFromAOG =
                traffic.cntrGPSOut =
                traffic.cntrIMUOut =
                traffic.cntrSteerIn = traffic.cntrSteerOut =
                traffic.cntrMachineOut = traffic.cntrMachineIn = 0;

            lblCurentLon.Text = longitude.ToString("N7");
            lblCurrentLat.Text = latitude.ToString("N7");

            if (traffic.cntrGPSIn > 9999) traffic.cntrGPSIn = 0;
        }

        private void RescanPorts()
        {
            string[] ports = System.IO.Ports.SerialPort.GetPortNames();

            if (ports.Length == 0)
            {
                lblSerialPorts.Text = "None";
            }
            else
            {
                for (int i = 0; i < ports.Length; i++)
                {
                    lblSerialPorts.Text = ports[i] + " ";
                }
            }
        }

        public void ConfigureNTRIP()
        {
            lblWatch.Text = "Wait GPS";
            lblMessages.Text = "Reading...";
            lblTotalMessageCount.Text = "0";
            lblNTRIP_IP.Text = "";
            lblMount.Text = "";

            aList.Clear();
            rList.Clear();
            lblMessages.Text = "Reading....";

            //start NTRIP if required
            isNTRIP_RequiredOn = Settings.Default.setNTRIP_isOn;
            isRadio_RequiredOn = Settings.Default.setRadio_isOn;

            if (isRadio_RequiredOn)
            {
                // Immediatly connect radio
                ntripCounter = 20;
            }

            //lblMount.Text = Properties.Settings.Default.setNTRIP_mount;

            if (isNTRIP_RequiredOn || isRadio_RequiredOn)
            {
                btnStartStopNtrip.Visible = true;
                btnStartStopNtrip.Visible = true;
                lblWatch.Visible = true;
                lblNTRIPBytes.Visible = true;
                lblToGPS.Visible = true;
                lblCount.Visible = true;
                lblMount.Visible = true;
                lblNTRIP_IP.Visible = true;
            }
            else
            {
                btnStartStopNtrip.Visible = false;
                btnStartStopNtrip.Visible = false;
                lblWatch.Visible = false;
                lblNTRIPBytes.Visible = false;
                lblToGPS.Visible = false;
                lblCount.Visible = false;
                lblMount.Visible = false;
                lblNTRIP_IP.Visible = false;
            }

            btnStartStopNtrip.Text = "Off";
        }

        private void btnDeviceManager_Click(object sender, EventArgs e)
        {
            Process.Start("devmgmt.msc");
        }

        private void deviceManagerToolStripMenuItem_Click(object sender, EventArgs e)
        {
            Process.Start("devmgmt.msc");
        }

        private void btnBringUpCommSettings_Click(object sender, EventArgs e)
        {
            SettingsCommunicationGPS();
            RescanPorts();
        }

        private void btnUDP_Click(object sender, EventArgs e)
        {
            SettingsUDP();
        }

        private void btnRunAOG_Click(object sender, EventArgs e)
        {
            StartAOG();
        }

        private void btnNTRIP_Click(object sender, EventArgs e)
        {
            SettingsNTRIP();
        }

        private void btnRadio_Click(object sender, EventArgs e)
        {
        }

        private void btnExit_Click(object sender, EventArgs e)
        {
            Close();
        }

        private void uDPToolStripMenuItem_Click(object sender, EventArgs e)
        {
            SettingsUDP();
        }

        private void pictureBox2_Click(object sender, EventArgs e)
        {
            Form f = Application.OpenForms["FormGPSData"];

            if (f != null)
            {
                f.Focus();
                f.Close();
                isGPSSentencesOn = false;
                return;
            }

            isGPSSentencesOn = true;

            Form form = new FormGPSData(this);
            form.Show(this);
        }

        private void btnRadio_Click_1(object sender, EventArgs e)
        {
            SettingsRadio();
        }

        private void btnWindowsShutDown_Click(object sender, EventArgs e)
        {
            DialogResult result3 = MessageBox.Show("Shutdown Windows For Realz ?",
                "For Sure For Sure ?",
                MessageBoxButtons.YesNo,
                MessageBoxIcon.Question,
                MessageBoxDefaultButton.Button2);

            if (result3 == DialogResult.Yes)
            {
                Process.Start("shutdown", "/s /t 0");
            }
        }

        private void toolStripGPSData_Click(object sender, EventArgs e)
        {
            Form f = Application.OpenForms["FormGPSData"];

            if (f != null)
            {
                f.Focus();
                f.Close();
                isGPSSentencesOn = false;
                return;
            }

            isGPSSentencesOn = true;

            Form form = new FormGPSData(this);
            form.Show(this);
        }

        private void radioToolStrip_Click(object sender, EventArgs e)
        {
            SettingsRadio();
        }

        private void btnSlide_Click(object sender, EventArgs e)
        {
            if (this.Width == 430)
            {
                this.Width = 700;
                isViewAdvanced = true;
                btnSlide.BackgroundImage = Properties.Resources.ArrowGrnLeft;
                lblMessages.Text = "Reading...";
            }
            else
            {
                this.Width = 430;
                isViewAdvanced = false;
                btnSlide.BackgroundImage = Properties.Resources.ArrowGrnRight;
                aList.Clear();
                rList.Clear();
                lblMessages.Text = "Reading...";
            }
        }

        private void cboxLogNMEA_CheckedChanged(object sender, EventArgs e)
        {
            isLogNMEA = cboxLogNMEA.Checked;
        }

    }
}

