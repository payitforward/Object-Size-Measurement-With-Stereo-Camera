using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.IO;
using System.Diagnostics;
using System.Threading;

namespace App
{
    public partial class Form1 : Form
    {
        static string path = System.IO.Directory.GetCurrentDirectory();
        static string final_path = path.Replace(@"\", @"\\");
        static string path1 = final_path + "\\control\\start\\start";
        static string path2 = final_path + "\\control\\close\\close";
        static string path3 = final_path + "\\control\\state\\state";
        static string path4 = final_path + "\\control\\manual\\manual";
        static string path5 = final_path + "\\control\\auto\\auto";
        static string duoi = ".txt";
        static string duoi1 = ".tif";
        static string image_path = final_path + "\\control\\image\\image";
        static int i1 = 0;
        int i2 = 0;
        int i3 = 0;
        int i4 = 0;
        int i5 = 0;
        int i6 = 0;



        public Form1()
        {
            Form f = new Form();
            { 
            f.StartPosition = FormStartPosition.CenterScreen;
            f.Size = new Size(400, 400);
            f.FormBorderStyle = FormBorderStyle.None;
            f.MinimizeBox = false;
            f.MaximizeBox = false;
            }
            Image im = Image.FromFile("box.jpg");
            PictureBox pb = new PictureBox();
            {
                pb.Dock = DockStyle.Fill;
                pb.Image = im;
                pb.Location = new Point(5, 5);
            }
            f.Controls.Add(pb);
            f.Show();
            var delay = Task.Delay(1000);
            delay.Wait();
            f.Close();
            InitializeComponent();
            button1.Enabled = false;
        }

        private void label2_Click(object sender, EventArgs e)
        {

        }

        private void button1_Click(object sender, EventArgs e)
        {
            string number3 = i3.ToString();
            string number6 = i6.ToString();
            string diachi3 = path3 + number3 + duoi;
            string anh = image_path + number6 + duoi1;
            File.CreateText(diachi3);

            string line1;
            string line2;
            string line3;
            // Read the file and display it line by line. 
            var delay = Task.Delay(400);
            delay.Wait();

            System.IO.StreamReader file1 = new System.IO.StreamReader(final_path + "\\distance1.txt");
            System.IO.StreamReader file2 = new System.IO.StreamReader(final_path + "\\distance2.txt");
            System.IO.StreamReader file3 = new System.IO.StreamReader(final_path + "\\distance3.txt");
            line1 = file1.ReadLine();
            line2 = file2.ReadLine();
            line3 = file3.ReadLine();
            textBox1.Text = line1;
            textBox2.Text = line2;
            textBox3.Text = line3;
            file1.Close();
            file2.Close();
            file3.Close();
            i3++;
            i6++;
            pictureBox1.Image = Image.FromFile(anh);
        }

        private void button2_Click(object sender, EventArgs e)
        {
            textBox4.BackColor = Color.Green;
            textBox4.Text = "ACTIVATE";
            string number1 = i1.ToString();
            string diachi1 = path1 + number1 + duoi; ;
            File.CreateText(diachi1);
            i1++;
            //File.Delete(path1);
        }

        private void button3_Click(object sender, EventArgs e)
        {
            textBox4.BackColor = Color.Yellow;
            textBox4.Text = ".....WAIT.....";
            button1.Enabled = false;
            string number2 = i2.ToString();
            string diachi2 = path2 + number2 + duoi;
            File.CreateText(diachi2);
            i2++;
        }

        private void button4_Click(object sender, EventArgs e)
        {
            textBox4.BackColor = Color.Green;
            textBox4.Text = ".....RUNNING.....";
            button1.Enabled = true;
            string number5 = i5.ToString();
            string diachi5 = path5 + number5 + duoi;
            File.CreateText(diachi5);
            textBox5.Text = "AUTO";
            i5++;
        }

        private void button5_Click(object sender, EventArgs e)
        {
            textBox4.BackColor = Color.Green;
            textBox4.Text = ".....RUNNING.....";
            button1.Enabled = true;
            string number4 = i4.ToString();
            string diachi4 = path4 + number4 + duoi;
            File.CreateText(diachi4);
            textBox5.Text = "MANUAL";
            i4++;
        }
    }
}
