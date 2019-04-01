## install.sh

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
