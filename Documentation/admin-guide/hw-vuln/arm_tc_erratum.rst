.. SPDX-License-Identifier: GPL-2.0

Arm TC erratum
==============

The Arm TC erratum affects a number of infrastructure and mobile 'big' cores
since Corex-A77 and Neoverse-N2. The erratum allows a malicious guest to
indirectly read and modify memory that was not permitted by the hypervisor.

Affected processors
-------------------

The erratum affects the following CPUs:
 * Cortex A77
 * Cortex A78
 * Cortex A78C
 * Cortex A78C
 * Cortex A710
 * Cortex X1
 * Cortex X1C
 * Cortex X2
 * Cortex X3
 * Cortex X4
 * Cortex X925
 * Neoverse N2
 * Neoverse V1
 * Neoverse V2

Whether a processor is affected or not can be read out from the arm_tc_erratum
vulnerability file in sysfs.

Related CVEs
------------

The following CVE entry is related to this issue:

   ==============  =====  ===================================================
   CVE-2025-11689         SOME STANDARD TEXT GOES HERE
   ==============  =====  ===================================================

Problem
-------

When performing a page table walk the CPU can cache the location of intermediate
entries to accelerate subsequent page table walks. The MMU configuration controls
the format of the page tables, such as the number of levels and which virtual
address bits are used in each stage of the walk. When executing a virtual machine
there are two sets of MMU configuration, one from the guest (stage1) and another
from the hypervisor to control the guest's view of physical addresses (stage2).

This erratum occurs when the CPU has cached two entries for the same part of
the page table walk. Matching one of these entries will cause the page table walker
to combine both entries and use the result. The arm architecture terms this
amalgamation. The conditions can be created using the guest's stage1 MMU controls,
and the combined entries allow an access that was not permitted by the hypervisor's
stage2. This only applies to the page table walker. Load and Store instructions
issued by the guest are still checked against stage2.

This allows the page table walker to access arbitrary memory as if it were a page
table entry. The page table walker may set the access-flag or dirty-bit if
hardware-management of these bits is enabled at stage1, and the arbitrary memory has
the 'DBM bit' set. In addition the page table walker may return the result of its
walk in the PAR_EL1 register, allowing the 'PA bits' of arbitrary memory to be read.

More detailed technical information is available in the Software Developer Errata
Note (SDEN) or each CPU.


Attack scenarios
----------------

Attacks against this erratum can be implemented from guest kernels.


Mitigation mechanism
--------------------

The workaround requires an implementation defined bit to be set by platform firmware
which disables the affected parts of the intermediate walk cache. This comes with a
significant performance cost for some workloads.
It is possible for platform firwmare to dynamically manage the state of this bit,
meaning only guests that could trigger the erratum are impacted. This dynamic management
by firmware is unable to prevent the SPE or TRBE features from triggering the erratum.

Firmware offers the following modes:
  ==================  =============================================================
  static-off          This is previous behaviour, the implementation defined bit
                      is not set, performance is not impacted, but the erratum is
                      not prevented by platform firmware.

  static-on           The implementation defined bit is set. Performance is
                      impacted. The erratum is prevented for all workloads and
                      features. This mode does not trap any registers and has
                      predictable performance.

  dynamic             Writes to the MMU configuration registers are intercepted by
                      platform firmware, and the implementation defined bit is set
                      if the erratum conditions can occur. Only affected
                      configurations are impacted. SPE and TRBE are disabled by
                      platform firmware to prevent the erratum occuring.

  dynamic-incomplete  The same as dynamic, but SPE and TRBE remain enabled. This
                      relies on the OS not to use SPE or TRBE in a configuration
                      that is affected by the erratum.
  ==================  =============================================================

The mode can be controlled by a firmware SMC-CC interface.


Kernel selection of mitigation mode
-----------------------------------

The kernel does not expose a command line option to control the specific mitigation
mode, instead the mode that gives the best performance and prevents the erratum is
chosen at boot.

If the erratum conditions can not occur due to the kernel's build configuration
(e.g. use of 64K pages), or the way the platform booted (e.g. booting at EL1
meaning no KVM guests can run), then the 'static-off' mode is selected.
This mode is also selected if 'mitigations=off' is added to the kernel command line.

If pKVM is in use, the dynamic mode is chosen as pKVM's stage2 for the host allows
the host to trigger the erratum conditions. This mode disables SPE and TRBE.

Otherwise 'dynamic-incomplete' is chosen as KVM does not currently allow SPE or
TRBE to be used in an affected configuration.

The mode selected by the kernel is exposed in the sysfs vulnerabilities files.
The options are.

  =============================================================
  Not Affected
  Vulnerable
  Mitigation: Off; kernel build or platform configuration
  Mitigation: Dynamic; pKVM enabled"
  Mitigation: Dynamic
  =============================================================

