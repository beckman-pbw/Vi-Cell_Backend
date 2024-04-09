using System;

namespace RfidExtensions
{
    public interface IGpioControl : IDisposable
    {
        bool Connect();

        void Disconnect();

        void SetIo(int number);

        void ClearIo(int number);

        string GetVersion();

        new void Dispose(); // declaring this again because importing the library in C++ using COM does not generate inherited interface members.
    }
}
