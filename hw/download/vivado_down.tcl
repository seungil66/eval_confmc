if {[info exists env(HOST)] == 0} {
     set HOST localhost
} else {
     set HOST $::env(HOST)
}
if {[info exists env(PORT)] == 0} {
     set PORT 3121
} else {
     set PORT $::env(PORT)
}
if {[info exists env(DEVICE)] == 0} {
     set DEVICE xc7a200t_0
} else {
     set DEVICE $::env(DEVICE)
}

if {[info exists env(BITFILE)] == 0} {
     set BITFILE ../bitstream/nexys_video.bit
} else {
     set BITFILE $::env(BITFILE)
}

if { [file exists ${BITFILE}] == 0 } {
   puts "${BITFILE} not found."
   exit
}

#puts ${BITFILE}
#########################################################################
if { [version -short]==2019.2 } {
    open_hw_manager
} elseif { [version -short]==2018.3 || [version -short]==2018.2 } {
    open_hw
}

connect_hw_server -url ${HOST}:${PORT} -allow_non_jtag
open_hw_target
disconnect_hw_server ${HOST}:${PORT}
connect_hw_server -url ${HOST}:${PORT} -allow_non_jtag
open_hw_target

current_hw_device [get_hw_devices ${DEVICE}]
refresh_hw_device -update_hw_probes false [lindex [get_hw_devices ${DEVICE}] 0]
set_property PROBES.FILE {} [get_hw_devices ${DEVICE}]
set_property FULL_PROBES.FILE {} [get_hw_devices ${DEVICE}]
set_property PROGRAM.FILE ${BITFILE} [get_hw_devices ${DEVICE}]
program_hw_devices [get_hw_devices ${DEVICE}]
refresh_hw_device [lindex [get_hw_devices ${DEVICE}] 0]
########################################################################
