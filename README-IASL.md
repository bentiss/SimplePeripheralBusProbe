Requirements
------------

- a Windows machine with a serial (I2C or SPI) device ACPI-enumerated
- time
- skills
- ```traceview.exe``` (provided by the WDK in tools\tracing\<Platform>)
- ```asl.exe``` (provided by the WDK in Tools\x64\ACPIVerify)
- ```iasl.exe``` (https://www.acpica.org/downloads/binary-tools)
- ```RWEverything``` (http://rweverything.phpnet.us)
- a way to boot a repair environment if things goes wrong (recovery disk, Windows disk, repair partition)

Instructions
------------

The guide for doing everything with Microsoft `asl.exe` is quite detailed. Use it where details are required.

This is as an extension because often the microsoft compiler does not know how to compile the `asl` it extracted.
Intel created a compiler that is much clearer in it's error messages and knows howto compile the `asl`-files.
Unfortunately you have to provide the binary files in order to decompile them.

Getting the Binary DSDT
=======================

Install `RWEverything`.
Once you have installed `RWEverything`, run it and from the `Access` menu, choose `ACPI Tables`. From the `ACPI Table` window that opens, first select the `DSDT` tab to make sure the `DSDT` table is active, then choose the `Save as Binary` button on toolbar (or `Shift+F2`).

You should then have a file called `DSDT.aml`

Decompiling
===========

To decompile the file with `iasl.exe` you need to call it with the full path.

In theory this could work:

```
> iasl.exe -d DSDT.aml
```

However in many cases `iasl` complains about missing dependencies. To get those you can extract the `SSDT`-tables using `RWEverything`. - Make sure to store them with a different extension (example `.bin`) than the `DSDT` to later be able to use easy wildcards.

Also this might still not be enough and you need additional info. You can add this info using a `refs.txt` file with the following content:

### refs.txt
```
External(MDBG, MethodObj, 1)
External(_GPE.MMTB, MethodObj, 0)
External(_SB.PCI0.LPCB.H_EC.ECWT, MethodObj, 2)
External(_SB.PCI0.LPCB.H_EC.ECRD, MethodObj, 1)
External(_SB.PCI0.LPCB.H_EC.ECMD, MethodObj, 1)
External(_SB.PCI0.PEG0.PEGP.SGPO, MethodObj, 2)
External(_SB.PCI0.GFX0.DD02._BCM, MethodObj, 1)
External(_SB.PCI0.SAT0.SDSM, MethodObj, 4)
External(_GPE.VHOV, MethodObj, 3)
External(_SB.PCI0.XHC.RHUB.TPLD, MethodObj, 2)
```

If the decompiler complains about duplicate definitions just remove them from this file.

With those files set up you should now be able to run a command similar to this:

```
> iasl.exe -e *.bin -fe refs.txt -d DSDT.aml
```

This should sucessfully decompile the code to a file called `DSDT.dsl`. Rename this file to a different name to avoid confusions with the original version for example `DSDT-mod.dsl`. Adjust this file just as explained in the original tutorial.

Compiling the `DSDT-mod.asl`
========================

Compiling is pretty straight forward:

```
> iasl.exe DSDT-mod.dsl
```

This produces a file `DSDT-mod.aml` - If you did not rename the original `DSDT` this might overwrite your `DSDT.aml` extracted earlier.

Loading the compiled `DSDT-mod.aml`
===================================

To load the file you just use the microsoft `asl.exe`

```
> asl.exe /loadtable DSDT-mod.aml
```