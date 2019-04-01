## install.sh
# Install      :
# cd /home/pi
# git clone https://github.com/rinalim/VolumeJoy
# cd VolumeJoy
# chmod 755 install.sh
# sudo ./install.sh
#
# Reference    :
# https://github.com/RetroPie/RetroPie-Setup/blob/master/scriptmodules/supplementary/runcommand/joy2key.py
# https://github.com/sana2dang/PauseMode

sudo cp ./libraspidmx.so.1 /usr/lib

sudo rm -rf /opt/retropie/configs/all/CompiGamepad/
sudo cp -f -r ./CompiGamepad /opt/retropie/configs/all/

sudo chmod +x /opt/retropie/configs/all/CompiGamepad/pngview

sudo sed -i '/compigamepad.py/d' /opt/retropie/configs/all/autostart.sh
sudo sed -i '1i\\/usr/bin/python /opt/retropie/configs/all/CompiGamepad/compigamepad.py /dev/ttyACM0 &' /opt/retropie/configs/all/autostart.sh

echo
echo "Setup Completed. Reboot after 3 Seconds."
sleep 3
sudo reboot
