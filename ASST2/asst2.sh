cd ~/cse3211/asst2/src
./configure
cd ~/cse3211/asst2/src
bmake
bmake install
cd ~/cse3211/asst2/src/kern/conf
./config ASST2
cd ../compile/ASST2
bmake depend
bmake
bmake install
cd ~/os161/root
sys161 kernel-ASST2 "p /testbin/asst2"
