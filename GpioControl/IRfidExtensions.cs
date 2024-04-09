using System;

namespace RfidExtensions
{
    /// <summary>
    /// Provides extensions to the RFID hardware controls.  NOTE: int return type was used in liu of bool type because
    /// the generated COM adapter used by managed C++ always gave a value of -1 regardless of the underlying boolean values.
    /// </summary>
    public interface IRfidExtensions : IDisposable
    {
        SignalState State { get; }

        /// <summary>
        /// Gets a zero value indicating that the GPIO hardware was found and a connection to it exists. Otherwise, negative 1 indicates an absent or failed connection.
        /// </summary>
        int GetIsHardwareConnected();

        /// <summary>
        /// Send the command to indicate that RFID programming station had a write failure.
        /// </summary>
        /// <returns>0 if the state changed, -1 if it did not. The latter may indicate a missing hardware connection.</returns>
        int SignalWriteFailure();

        /// <summary>
        /// Send the command to indicate that RFID programming station had a write success.
        /// </summary>
        /// <returns>0 if the state changed, -1 if it did not. The latter may indicate a missing hardware connection.</returns>
        int SignalWriteSuccess();

        /// <summary>
        /// Send the command to indicate that RFID programming station is operating nominally.
        /// </summary>
        /// <returns>0 if the state changed, -1 if it did not. The latter may indicate a missing hardware connection.</returns>
        int SignalOperationNominal();

        new void Dispose(); // declaring this again because importing the library in C++ using COM does not generate inherited interface members.
    }
}
