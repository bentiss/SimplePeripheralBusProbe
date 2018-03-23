
I2C/SPI probe for Windows 10
============================

This driver is **extremely** experimental and should only be used by people knowing what they are doing and not afraid of destroying their Windows install.

The purpose of this project is to insert a pass-through driver between the target I2C or SPI device and its driver.
This pass-through device logs each serial transaction made by the driver. It's useful to watch the raw sequences to ensure the driver works properly.

This code is a mix of 2 drivers found in the Windows drivers sample repository, so the license is the Microsoft one (see LICENSE).

Often `asl.exe` fails at compiling its extracted file. The `iasl.exe` provided by Intel is much better at this job. For a guide on how to use this compiler instead use: [README-IASL.md](./README-IASL.md) (this text is just an extension and not a replacement for the explanations here)

Requirements
------------

- a Windows machine with a serial (I2C or SPI) device ACPI-enumerated
- time
- skills
- ```traceview.exe``` (provided by the WDK in tools\tracing\<Platform>)
- ```asl.exe``` (provided by the WDK in Tools\x64\ACPIVerify)
- a way to boot a repair environment if things goes wrong (recovery disk, Windows disk, repair partition)

Overloading the DSDT
--------------------

Windows 8+ allows to overload the DSDT through the ```/loadtable``` option of asl.exe.
See https://msdn.microsoft.com/en-us/windows/hardware/drivers/bringup/microsoft-asl-compiler

The first step is to retrieve the current DSDT (or SSDT if the device is declared in the SSDT). Run in a cmd.exe process as an Administrator:

You will find WDK Tools under ```C:\Program Files (x86)\Windows Kits\10\Tools\x64\ACPIVerify``` if you didn't changed the default installation path.

```
asl.exe /tab=DSDT
```

This will produce a DSDT.ASL file in the current directory.
Next step is to increment the version of the table (the last number in ```DefinitionBlock()```).

Then recompile the DSDT and fix any errors

```
asl.exe DSDT.ASL
```

When the errors are fixed, you can load the table for the next boot:

```
asl.exe /loadtable -v DSDT.AML
```

Make sure the version is the one you just incremented.
The table will be stored in the registry under ```HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\ACPI\Parameters\DSDT\XXX\YYY\ZZZ``` where ```XXX```,```YYY```,```ZZZ``` will depend from your edited DSDT.

Before rebooting, make sure Windows will allow non signed drivers (*Be cautious when enabling this, any driver can now be installed, even bad ones*. You have been warned).

```
bcdedit /set testsigning on
```

Reboot. If the device reboots and a new dump of the DSDT shows your incremented version number, you are good to continue. If not, follow the next section to fix the broken boot.

What if the DSDT is wrong and I get the blue screen ACPI_BIOS_ERROR at boot?
----------------------------------------------------------------------------

As mentioned previously, the overloaded DSDT is just stored in the registry, so the trick is to edit the registry and remove the keys.

So if Windows doesn't start, you need first to boot into the recovery environment. 
For that, the simplest way is to boot the repair partition or use a recovery disk (search Microsoft's site for how to build one).
For OEM installs, you just need to boot again on Windows and it will start the recovery environment.

Once you are in the recovery environment, there is a trick (thanks this [blog](http://www.techrepublic.com/blog/windows-and-office/use-the-recovery-drive-command-prompt-to-edit-the-registry-or-recover-data/) and others).

The key is that when you are in the recovery, if you start ```regedit```, the registry that will be edited is the one from the recovery environment, not the one from the installation.

So once ```regedit``` is started, you need to add the keys from the current installation by using ```Load Hive...``` from the ```File``` menu. If the menu entry ```Load Hive...``` is disabled, please select a node (e.g. `HKEY_LOCAL_MACHINE`) and then the menu entry should be available.
You will need to load the file ```%windir%\system32\config\SYSTEM```. You will be prompted to set a key name for this loaded hive. Name it as you wish (e.g. ```Origin```).
In this newly loaded key, go down to ```HKEY_LOCAL_MACHINE\Origin\HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\ACPI\Parameters\DSDT\XXX\YYY\ZZZ``` (as mentioned previously) and simply remove the ```ZZZ``` folder.

Reboot and enjoy a working Windows.

Insert the Probe in the DSDT
----------------------------

Now that you have a working environment for overloading the DSDT, we can start inserting the probe in order to be able to dump traces.

Let's say you have a HID over I2C device not working properly and you want to check the I2C traces.

The DSDT will look like (example taken from the [HID over I2C specification](https://msdn.microsoft.com/en-us/library/windows/hardware/dn642101(v=vs.85).aspx)):

```
Scope (\_SB) {

//--------------------
//  General Purpose I/O, ports 0...127
//--------------------

    
Device(HIDI2C_DEVICE1) {
    Name(_ADR,0)
    Name (_HID, "MSFT1234”) 
    Name (_CID, "PNP0C50") 
    Name (_UID, 3) 

    Method(_CRS, 0x0, NotSerialized)
    {
        Name (RBUF, ResourceTemplate ()
        {
        // Address 0x07 on I2C-X (OEM selects this address)
       //IHV SPECIFIC I2C3 = I2C Controller; TGD0 = GPIO Controller; 
        I2CSerialBus (0x07, ControllerInitiated, 100000,AddressingMode7Bit, "\\_SB.I2C3",,,,)
        GpioInt(Level, ActiveLow, Exclusive, PullUp, 0, "\\_SB. TGD0", 0 , ResourceConsumer, , ) {40}  
        })
     Return(RBUF)
     }



     Method(_DSM, 0x4, NotSerialized)
     {
        // BreakPoint
        Store ("Method _DSM begin", Debug)

        // DSM UUID
        switch(ToBuffer(Arg0))
        {		
            // ACPI DSM UUID for HIDI2C
            case(ToUUID("3CDFF6F7-4267-4555-AD05-B30A3D8938DE"))
            {
                // DSM Function
                switch(ToInteger(Arg2))
                {
                    // Function 0: Query function, return based on revision
                    case(0)
                    {
                        // DSM Revision
                        switch(ToInteger(Arg1))
                        {
                            // Revision 1: Function 1 supported
                            case(1)
                            {
                                Store ("Method _DSM Function Query", Debug)
                                Return(Buffer(One) { 0x03 })
                            }
                            
                            default
                            {
                                // Revision 2+: no functions supported
                                Return(Buffer(One) { 0x00 })
                            }
                        }
                    }

                    // Function 1 : HID Function
                    case(1)
                    {
                        Store ("Method _DSM Function HID", Debug)
                        
                        // HID Descriptor Address
                        Return(0x0001)
                    }
                    
                    default
                    {
                        // Functions 2+: not supported
                    }
                }
            }
            
            default
            {
                // No other GUIDs supported
                Return(Buffer(One) { 0x00 })
            }
        }
    }
}
```

We are interested here in the ```_CRS``` method which describes the ressources used by the device.

In the same scope (```\_SB```) we are going to add one new I2C device that uses the same I2C target (found in ```spbProbe.asl```):

```
Device(SPB1)
{
    Name(_HID, "PROBE01")
    Name(_UID, 1)
	Method(_CRS, 0x0, NotSerialized)
    {
        Name (RBUF, ResourceTemplate ()
        {
            I2CSerialBus (0x07, ControllerInitiated, 100000,AddressingMode7Bit, "\\_SB.I2C3",,,,)
        })
        Return(RBUF)
    }

}
```

And we edit the current device to point to this one instead:

```
Device(HIDI2C_DEVICE1) {
    Name(_ADR,0)
    Name (_HID, "MSFT1234”) 
    Name (_CID, "PNP0C50") 
    Name (_UID, 3) 

    Method(_CRS, 0x0, NotSerialized)
    {
        Name (RBUF, ResourceTemplate ()
        {
        // Address 0x07 on I2C-X (OEM selects this address)
       //IHV SPECIFIC I2C3 = I2C Controller; TGD0 = GPIO Controller; 
        //I2CSerialBus (0x07, ControllerInitiated, 100000,AddressingMode7Bit, "\\_SB.I2C3",,,,)
        I2CSerialBus (0x07, ControllerInitiated, 100000,AddressingMode7Bit, "\\_SB.SPB1",,,,)
        GpioInt(Level, ActiveLow, Exclusive, PullUp, 0, "\\_SB. TGD0", 0 , ResourceConsumer, , ) {40}  
        })
     Return(RBUF)
     }

```

Do not forget to increment the DSDT version. Then recompile the DSDT and load it (```asl /loadtable DSDT.AML```).

Reboot.

In the ```Device Manager```, there should be a new device with an exclamation mark (```Unknown Device```) with a path ```ACPI\SPBPROBE\1``` in the ```Details``` panel of the device.

Load the driver from this repository (after compiling it or by downloading it from the [releases page](https://github.com/bentiss/SimplePeripheralBusProbe/releases)). You will get a big red screen telling you Windows can't trust the publisher of the driver, which is true. But if you are here, well you know what you are doing so just hit ```Install it anyway```.

Now the device should show up in the ```system devices``` list and will have the name ```I1C/SPI Probe Controller Driver```.
If you did not messed up, the target device (the HID over I2C device we are targeting) should still be working, and the probe must also say that it is working properly.

Get traces from the probe
-------------------------

Run ```traceview.exe``` as an Administrator. Then ```File``` -> ```Create New Log Session...```.

Chose the .pdb from the driver in the ```Add provider``` window. Then ```Next```.
By default, you will only have the real time display selected. If you want to store and process the logs, you want to also log the data to ta file.

For better traces, before hitting ```Finish```, disable the targeted device in Device Manager so that the logs start as if the device have just been plugged in.

> **Note:** By default, the traces are dumped as ```Error``` level. So you don't need to set any flags.
> If you run into problems and need to debug the probe, you can lower the debug level here to trace what is happening in the driver.

Convert the traces into a text file
-----------------------------------

When you record the logs, the traces are not in a text file, which doesn't help much to look at them.

```traceview``` allows to convert them into text.

First, you need to export the dictionary from the pdb file:

```
traceview.exe -parsepdb spbProbe.pdb
```

Then you can dump the actual log file:

```
traceview.exe -process LogSession_mmddyy_hhmmss.etl -o myFirstLogs.txt
```

(amend ```LogSession_mmddyy_hhmmss.etl``` and ```myFirstLogs.txt``` to match your needs).

That's it. Now ```myFirstLogs.txt``` contains the logs and you can do an analysis of where the problem is.
