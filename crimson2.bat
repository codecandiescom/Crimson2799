#!/bin/csh
# First, create the auto-reboot file 
touch ./autoreboot.flag
rm ./boot.flag

# now loop while the flag exists
# NOTE: the mud server removes the flag file initially
# but creates it after it successfully boots, thus
# preventing an infinite crash on boot problem

while (-e ./autoreboot.flag)
  ./crimson2.exe
  
  # at this point we can check for boot.flag file
  # if it exists the server crashed while booting
  # and we should perhaps email the admins.
  #
  # if (-e ./boot.flag)
  #   more msg/crash.msg | mail cryogen@infoserve.net
  # end
end
