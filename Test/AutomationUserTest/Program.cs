using System;
using System.Runtime.InteropServices;

namespace AutomationUserTest
{
    class Program
    {
        [DllImport("AUCred.dll")]
        static extern IntPtr AutomationPassword();
        [DllImport("AUCred.dll")]
        static extern IntPtr AutomationUsername();

        static void Main(string[] args)
        {
            IntPtr pw_ptr = AutomationPassword();
            string pwd = "";
            if (pw_ptr == IntPtr.Zero)
                pwd = null;
            else
                pwd = Marshal.PtrToStringAnsi(pw_ptr);

            string uname = "";
            IntPtr un_ptr = AutomationUsername();
            if (un_ptr == IntPtr.Zero)
                uname = null;
            else
                uname = Marshal.PtrToStringAnsi(un_ptr);

            Console.WriteLine($@"Username: {uname} Password: {pwd}");
        }
    }
}
