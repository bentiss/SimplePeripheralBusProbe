//
// Test controller device node. 
//
// For a peripheral driver to access this controller 
// via SPB it must specify the ACPI device path within 
// the I2CSerialBus (or SPISerialBus) macro. Depending 
// on the scope this looks something like \_SB.I2C. See
// spbtesttool.asl for an example.
//
Device(I2C)
{
    Name(_HID, "PROBE01")
    Name(_UID, 1)
	Method(_CRS, 0x0, NotSerialized)
    {
        Name (RBUF, ResourceTemplate ()
        {
            //
            // Sample I2C and GPIO resources. Modify to match your
            // platform's underlying controllers and connections.
            // \_SB.I2C and \_SB.GPIO are paths to predefined I2C.
            //
            I2CSerialBus(0x1D, ControllerInitiated, 400000, AddressingMode7Bit, "\\_SB.I2C", , )
			//SpiSerialBus (0x0001, PolarityLow, FourWireMode, 0x10,
			//	ControllerInitiated, 0x007A1200, ClockPolarityLow,
			//	ClockPhaseFirst, "\\_SB.SP1",
			//	0x00, ResourceConsumer, ,)
        })
        Return(RBUF)
    }

}