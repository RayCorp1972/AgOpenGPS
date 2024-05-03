using System;
using System.Globalization;
using System.Windows.Forms;

namespace AgOpenGPS
{
    public partial class FormAkkoord : Form
    {
        public FormAkkoord(FormGPS form)
        {
            InitializeComponent();
        }

        private void FormAkkoord_Load(object sender, EventArgs e)
        {

        }

        private void button1_Click(object sender, EventArgs e)
        {
            Close();
        }

        private void button2_Click(object sender, EventArgs e)
        {
            System.Environment.Exit(1);
        }
    }
}
