using System.IO.Ports;
using System.Threading;

namespace RfidExtensions
{
    public class GpioControl : IRfidExtensions, IGpioControl
    {
        private const int BaudRate = 9600;
        private const int StepDurationMs = 500;
        private const int NotConnected = -1;
        private const int Connected = 0;
        private const int NoStateChange = -1;
        private const int StateChanged = 0;
        private const string PortNamePrefix = "COM";
        private const string SetIoCommand = "gpio set {0}\r";
        private const string ClearIoCommand = "gpio clear {0}\r";
        private const string ExpectedVersionResponse = "00000008";
        private const string GetVersionCommand = "ver\r";
        private SerialPort _serialPort;
        private bool _isGpioFound;

        public SignalState State { get; private set; } = SignalState.Unknown;

        public bool IsHardwareConnected  { get { return _isGpioFound && _serialPort != null && _serialPort.IsOpen && VerifyConnection(); } }

        /// <summary>
        /// Gets a zero value indicating that the GPIO hardware was found and a connection to it exists. Otherwise, negative 1 indicates an absent or failed connection.
        /// </summary>
        public int GetIsHardwareConnected()
        {
            bool isTrue = IsHardwareConnected;
            //Console.WriteLine("\nGpioControl GetIsHardwareConnected: " + isTrue);
            return isTrue ? Connected : NotConnected;
        }


        public GpioControl()
        {
            InitSerialPort();
        }

        /// <summary>
        /// Send the command to indicate that RFID programming station is operating nominally.
        /// </summary>
        /// <returns>0 if the state changed, -1 if it did not. The latter may indicate a missing hardware connection.</returns>
        public int SignalOperationNominal()
        {
            if (!Connect())
                return NoStateChange;

            if (State == SignalState.ContinuousHigh)
                return NoStateChange;

            State = SignalState.ContinuousHigh;

            // set the continuous output high
            _serialPort.DiscardInBuffer();
            SetIo(Io.Default.ContinuousHigh);
            _serialPort.DiscardOutBuffer();

            return StateChanged;
        }

        /// <summary>
        /// Send the command to indicate that RFID programming station had a write failure.
        /// </summary>
        /// <returns>0 if the state changed, -1 if it did not. The latter may indicate a missing hardware connection.</returns>
        public int SignalWriteFailure()
        {
            if (!Connect())
                return NoStateChange;

            if (State == SignalState.AllLow)
                return NoStateChange;

            State = SignalState.AllLow;

            // set continuous output low
            ClearAllIo();

            return StateChanged;
        }

        /// <summary>
        /// Send the command to indicate that RFID programming station had a write success.
        /// </summary>
        /// <returns>0 if the state changed, -1 if it did not. The latter may indicate a missing hardware connection.</returns>
        public int SignalWriteSuccess()
        {
            if (!Connect())
                return NoStateChange;

            if (State == SignalState.ContinuousAndStepHigh)
                return NoStateChange;

            var nominalResult = SignalOperationNominal();
            if (nominalResult == NoStateChange && State != SignalState.ContinuousHigh)
            {
                return NoStateChange;
            }

            State = SignalState.ContinuousAndStepHigh;

            // pulse the stepped output high/low
            _serialPort.DiscardInBuffer();
            SetIo(Io.Default.SteppedHigh);
            Thread.Sleep(StepDurationMs);
            ClearIo(Io.Default.SteppedHigh);
            _serialPort.DiscardOutBuffer();

            State = SignalState.ContinuousHigh;

            return StateChanged;
        }

        public void SetIo(int number)
        {
            if (_serialPort == null)
                return;

            WriteToComPort(string.Format(SetIoCommand, number));
        }

        public void ClearIo(int number)
        {
            if (_serialPort == null)
                return;

            WriteToComPort(string.Format(ClearIoCommand, number));
        }

        /// <summary>
        /// This is used by <see cref="IsHardwareConnected"/> to verify the connection, it must avoid use of
        /// <see cref="IsHardwareConnected"/> and <see cref="VerifyConnection"/>
        /// </summary>
        /// <returns>The version string</returns>
        public string GetVersion()
        {
            try
            {
                _serialPort.DiscardInBuffer();
                WriteToComPort(GetVersionCommand);

                string response = _serialPort.ReadExisting();
                if (response.Length > 11)
                {
                    response = response.Substring(5, 8);
                }

                _serialPort.DiscardOutBuffer();

                //Console.WriteLine("\nGpioControl version response: " + response);
                return response;
            }
            catch
            {
                //Console.WriteLine("\nGpioControl: exception in GetVersion()");
                return null;
            }
        }

        public bool Connect()
        {
            if (_serialPort == null)
            {
                //Console.WriteLine("\nGpioControl: Serial port is null");
                return false;
            }

            if (IsHardwareConnected)
            {
                return true;
            }

            try
            {
                var portName = FindGpioPortName();
                if (string.IsNullOrEmpty(portName))
                {
                    //Console.WriteLine("\nGpioControl failed to find the hardware port.");
                    return false;
                }

                //Console.WriteLine("\nGpioControl found hardware on port " + portName);
                _isGpioFound = true;

                _serialPort.PortName = portName;
                _serialPort.Open();
                return true;
            }
            catch
            {
                //Console.WriteLine("\nGpioControl: exception in Connect()");
                return false;
            }
        }

        public void Disconnect()
        {
            _serialPort?.Close();
        }

        private void WriteToComPort(string message)
        {
            _serialPort.Write(message);
            // This sleep was used after writes in the OEM sample code
            Thread.Sleep(10);
        }

        private string FindGpioPortName()
        {
            var availSerialPorts = SerialPort.GetPortNames();
            foreach (var portName in availSerialPorts)
            {
                if (ProbeIsGpioOnPort(portName))
                {
                    return portName;
                }
            }
            return string.Empty;
        }

        private bool ProbeIsGpioOnPort(string portname)
        {
            if (_serialPort == null)
                return false;

            var origName = _serialPort.PortName;
            _serialPort.PortName = portname;

            try
            {
                _serialPort.Open();

                return VerifyConnection();
            }
            catch
            {
                return false;
            }
            finally
            {
                // put the port back into the state it was at before it was used here.
                Disconnect();
                _serialPort.PortName = origName;
            }
        }

        private bool VerifyConnection()
        {
            var version = GetVersion();
            // Test that the received version string is something like we expect.
            return !string.IsNullOrEmpty(version) && ExpectedVersionResponse.StartsWith(version);
        }

        private void InitSerialPort()
        {
            if (_serialPort != null && _serialPort.IsOpen || _isGpioFound)
            {
                return;
            }

            _isGpioFound = false;
            _serialPort = new SerialPort();            
            _serialPort.BaudRate = BaudRate;
        }

        private void ClearAllIo()
        {
            if (!IsHardwareConnected)
            {
                return;
            }

            _serialPort.DiscardInBuffer();
            ClearIo(Io.Default.ContinuousHigh);
            ClearIo(Io.Default.SteppedHigh);
            _serialPort.DiscardOutBuffer();
        }

        #region IDisposable Support
        private bool disposedValue = false; // To detect redundant calls

        protected virtual void Dispose(bool disposing)
        {
            if (!disposedValue)
            {
                if (disposing)
                {
                    ClearAllIo();

                    if (_serialPort != null)
                    {
                        _serialPort.Dispose();
                        _serialPort = null;
                    }
                }

                // TODO: free unmanaged resources (unmanaged objects) and override a finalizer below.
                // TODO: set large fields to null.

                disposedValue = true;
            }
        }

        // This code added to correctly implement the disposable pattern.
        public void Dispose()
        {
            // Do not change this code. Put cleanup code in Dispose(bool disposing) above.
            Dispose(true);
            // TODO: uncomment the following line if the finalizer is overridden above.
            // GC.SuppressFinalize(this);
        }
        #endregion
    }
}
