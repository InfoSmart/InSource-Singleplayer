/*===============================================================================================
 Load from memory example
 Copyright (c), Firelight Technologies Pty, Ltd 2004-2011.

 This example is simply a variant of the play sound example, but it loads the data into memory
 then uses the 'load from memory' feature of System::createSound.
 ===============================================================================================*/

using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;
using System.Data;
using System.Runtime.InteropServices;
using System.IO;

namespace loadfrommemory
{
	public class Form1 : System.Windows.Forms.Form
	{
        private FMOD.System  system  = null;
        private FMOD.Sound   sound   = null;
        private FMOD.Channel channel = null;

        private byte[] audiodata;

        private System.Windows.Forms.Label label;
        private System.Windows.Forms.Button playButton;
        private System.Windows.Forms.Button pauseButton;
        private System.Windows.Forms.Button exit_button;
        private System.Windows.Forms.StatusBar statusBar;
        private System.Windows.Forms.Timer timer;
        private System.ComponentModel.IContainer components;

		public Form1()
		{
			InitializeComponent();
		}

		protected override void Dispose( bool disposing )
		{
            if( disposing )
            {
                FMOD.RESULT     result;
            
                /*
                    Shut down
                */
                if (sound != null)
                {
                    result = sound.release();
                    ERRCHECK(result);
                }
                if (system != null)
                {
                    result = system.close();
                    ERRCHECK(result);
                    result = system.release();
                    ERRCHECK(result);
                }

                if (components != null) 
                {
                    components.Dispose();
                }
            }
            base.Dispose( disposing );
		}

		#region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
            this.components = new System.ComponentModel.Container();
            this.label = new System.Windows.Forms.Label();
            this.playButton = new System.Windows.Forms.Button();
            this.pauseButton = new System.Windows.Forms.Button();
            this.exit_button = new System.Windows.Forms.Button();
            this.statusBar = new System.Windows.Forms.StatusBar();
            this.timer = new System.Windows.Forms.Timer(this.components);
            this.SuspendLayout();
            // 
            // label
            // 
            this.label.Location = new System.Drawing.Point(16, 16);
            this.label.Name = "label";
            this.label.Size = new System.Drawing.Size(264, 32);
            this.label.TabIndex = 10;
            this.label.Text = "Copyright (c) Firelight Technologies 2004-2011";
            this.label.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            // 
            // playButton
            // 
            this.playButton.Location = new System.Drawing.Point(8, 56);
            this.playButton.Name = "playButton";
            this.playButton.Size = new System.Drawing.Size(136, 32);
            this.playButton.TabIndex = 25;
            this.playButton.Text = "Play";
            this.playButton.Click += new System.EventHandler(this.playButton_Click);
            // 
            // pauseButton
            // 
            this.pauseButton.Location = new System.Drawing.Point(152, 56);
            this.pauseButton.Name = "pauseButton";
            this.pauseButton.Size = new System.Drawing.Size(136, 32);
            this.pauseButton.TabIndex = 26;
            this.pauseButton.Text = "Pause/Resume";
            this.pauseButton.Click += new System.EventHandler(this.pauseButton_Click);
            // 
            // exit_button
            // 
            this.exit_button.Location = new System.Drawing.Point(110, 96);
            this.exit_button.Name = "exit_button";
            this.exit_button.Size = new System.Drawing.Size(72, 24);
            this.exit_button.TabIndex = 27;
            this.exit_button.Text = "Exit";
            this.exit_button.Click += new System.EventHandler(this.exit_button_Click);
            // 
            // statusBar
            // 
            this.statusBar.Location = new System.Drawing.Point(0, 123);
            this.statusBar.Name = "statusBar";
            this.statusBar.Size = new System.Drawing.Size(292, 24);
            this.statusBar.TabIndex = 28;
            // 
            // timer
            // 
            this.timer.Enabled = true;
            this.timer.Interval = 10;
            this.timer.Tick += new System.EventHandler(this.timer_Tick);
            // 
            // Form1
            // 
            this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
            this.ClientSize = new System.Drawing.Size(292, 147);
            this.Controls.Add(this.statusBar);
            this.Controls.Add(this.exit_button);
            this.Controls.Add(this.pauseButton);
            this.Controls.Add(this.playButton);
            this.Controls.Add(this.label);
            this.Name = "Form1";
            this.Text = "Load From Memory Example";
            this.Load += new System.EventHandler(this.Form1_Load);
            this.ResumeLayout(false);

        }
		#endregion

		[STAThread]
		static void Main() 
		{
			Application.Run(new Form1());
		}

        private void Form1_Load(object sender, System.EventArgs e)
        {
            uint                   version = 0;
            FMOD.RESULT            result;
            int                    length;
            FMOD.CREATESOUNDEXINFO exinfo = new FMOD.CREATESOUNDEXINFO();

            /*
                Global Settings
            */
            result = FMOD.Factory.System_Create(ref system);
            ERRCHECK(result);

            result = system.getVersion(ref version);
            ERRCHECK(result);
            if (version < FMOD.VERSION.number)
            {
                MessageBox.Show("Error!  You are using an old version of FMOD " + version.ToString("X") + ".  This program requires " + FMOD.VERSION.number.ToString("X") + ".");
                Application.Exit();
            }

            result = system.init(1, FMOD.INITFLAGS.NORMAL, (IntPtr)null);
            ERRCHECK(result);       

            length = LoadFileIntoMemory("../../../../../examples/media/wave.mp3");

            exinfo.cbsize = Marshal.SizeOf(exinfo);
            exinfo.length = (uint)length;

            result = system.createSound(audiodata, (FMOD.MODE.HARDWARE | FMOD.MODE.OPENMEMORY), ref exinfo, ref sound);
            ERRCHECK(result);
        }

        private void playButton_Click(object sender, System.EventArgs e)
        {
            FMOD.RESULT result;

            result = system.playSound(FMOD.CHANNELINDEX.FREE, sound, false, ref channel);
            ERRCHECK(result);
        }

        private void pauseButton_Click(object sender, System.EventArgs e)
        {
            FMOD.RESULT result;
            bool paused = false;

            if (channel != null)
            {
                result = channel.getPaused(ref paused);
                ERRCHECK(result);
                result = channel.setPaused(!paused);
                ERRCHECK(result);
            }
        }

        private void exit_button_Click(object sender, System.EventArgs e)
        {
            Application.Exit();
        }

        private void timer_Tick(object sender, System.EventArgs e)
        {
            FMOD.RESULT result;
            uint ms      = 0;
            uint lenms   = 0;
            bool playing = false;
            bool paused  = false;

            if (channel != null)
            {
                result = channel.isPlaying(ref playing);
                if ((result != FMOD.RESULT.OK) && (result != FMOD.RESULT.ERR_INVALID_HANDLE))
                {
                    ERRCHECK(result);
                }

                result = channel.getPaused(ref paused);
                if ((result != FMOD.RESULT.OK) && (result != FMOD.RESULT.ERR_INVALID_HANDLE))
                {
                    ERRCHECK(result);
                }

                result = channel.getPosition(ref ms, FMOD.TIMEUNIT.MS);
                if ((result != FMOD.RESULT.OK) && (result != FMOD.RESULT.ERR_INVALID_HANDLE))
                {
                    ERRCHECK(result);
                }

                result = sound.getLength(ref lenms, FMOD.TIMEUNIT.MS);
                if ((result != FMOD.RESULT.OK) && (result != FMOD.RESULT.ERR_INVALID_HANDLE))
                {
                    ERRCHECK(result);
                }
            }

            statusBar.Text = "Time " + (ms / 1000 / 60) + ":" + (ms / 1000 % 60) + ":" + (ms / 10 % 100) + "/" + (lenms / 1000 / 60) + ":" + (lenms / 1000 % 60) + ":" + (lenms / 10 % 100) + " : " + (paused ? "Paused " : playing ? "Playing" : "Stopped");

            if (system != null)
            {
                system.update();
            }
        }


        private int LoadFileIntoMemory(string filename)
        {
            int length;

            FileStream fs = new FileStream(filename, FileMode.Open, FileAccess.Read);

            audiodata = new byte[fs.Length];

            length = (int)fs.Length;

            fs.Read(audiodata, 0, length);
            
            fs.Close();

            return length;
        }


        private void ERRCHECK(FMOD.RESULT result)
        {
            if (result != FMOD.RESULT.OK)
            {
                timer.Stop();
                MessageBox.Show("FMOD error! " + result + " - " + FMOD.Error.String(result));
                Environment.Exit(-1);
            }
        }
	}
}
