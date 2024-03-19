# OK_Cloud_Streamer
An Open-source CloudXR 4 client, based on OpenXR with OK performance and features.

![OK_Cloud_Streamer](https://github.com/BattleAxeVR/OK_Cloud_Streamer/assets/11604039/89819682-e770-4f56-b47f-4752f4646db0)

![image](https://github.com/BattleAxeVR/OK_Cloud_Streamer/assets/11604039/b176601e-75b2-49f8-8154-93ebf30cc419)

![image](https://github.com/BattleAxeVR/OK_Cloud_Streamer/assets/11604039/3c083d80-1203-4a94-896f-0148aa47d31f)


Video showing Link Sharpening on/off:

https://twitter.com/BattleAxeVR/status/1769914456786473179

------
LEGAL:

MIT License: I was told by Nvidia reps on their forums that it's ok if I share the SteamVR plugin installer and libraries for CXR, as long as I use a permissive license such as MIT, which I am. 

All rights reserved and credit due to NV for their CloudXR library. 

WARRANTY: THERE IS NONE. USE THIS SOFTWARE ENTIRELY AT YOUR OWN RISK. I'M SERIOUS, if a lag spike causes you to drop your sword, you're on your own! 

But seriously, if you download the code here or an APK, just assume it's a) got bugs in it, b) could overheat your PC or Quest if you run settings too high, or you could have a large cloud server bill. None of these are my responsibility or fault. BE WARNED.

------
Goals:

-Provide an Open Source OpenXR-based client for CloudXR 4.0

https://developer.nvidia.com/cloudxr-sdk

-Graphics library is Facebook IGL:

https://github.com/BattleAxeVR/igl/tree/ok_cloud_streamer


-Support any and fall Quest (or other) standalone features available over OpenXR, such as eye-tracking, full body tracking simultaneous hands + controller tracking, via the custom non-VR Controller input data and server-side API functionality. 

-The direct server-side API can also enable a PC game/app to obviate the need for any OpenXR compositor on the PC side entirely, reducing overhead, synchronization and latency. However the goal is to use Monado or other open source solutions as a truly open source OpenXR runtime on PC that this client can connect to. Such a server will be included in this repo for a full A-to-Z solution.

-The client being open source means you can enable platform-wide features for PC VR gaming, such as waist-oriented locomotion (a software-only version of Decamove). 

------
Design Philosophy:

-Keep things as simple and "Plug n Play" as possible.

-Reduce clicks ("time to fun") needed to achieve something to a bare minimum, ideally zero. 

For ex: the first time you start the app, you set the IP address once and forget about it. Subsequent times can automatically log-in to the same IP. This IP and auto-connect feature can be changed manually using the UI control panel.

-Support as many pieces of hardware and user configurations as possible: input bindings remapping, multiple joysticks (paired via Bluetooth or USB port), Android gamepads, HOTAS, Steering wheels. This may be achieved via either SDL3 or AnyHere USB forwarding to PC (for 1:1 support on remote PC as if it were connected locally, w/o any code needed).

See here:

https://twitter.com/BattleAxeVR/status/1683233951614107648

Full Source + prebuilt APKs/.exe for the waist-loco demo (which will be ported into the client here shortly):

https://github.com/BattleAxeVR/OpenXR-SDK-Source

------
Build from source:

-Open client/Android in Android Studio (latest). Built it. For release, you will need to sign it as debug or create your own local keystore. The instructions how to do this are out of scope for this repo (just google it).

------

Running the OK Cloud Streamer app:

-First you must find and write the IP address of your server (the PC running SteamVR) in the JSON configuration file provided in client/config. You can find your IP using a command prompt and typing "ipconfig". It's usually a 192.X.X.X or similar. You can also specify an IP on the internet (indeed that's the purpose of this app, to work over local wifi or the cloud).

-Next, install the APK using ADB or Sidequest / Applab / whatever. Run it once, it will make the app's directory (but fail to connect). It may ask for microphone permission (not implemented yet, but soon).

-Next, install the JSON config file using the batch file or SideQuest/ADB to put it in the app's files/ folder.

-Install the CloudXR 4.0 server plugin on the PC running SteamVR (you may have to uninstall/reinstall or switch to Beta SteamVR). To get audio recording it also needs the server plugin configuration. See the README in the OVR sample, it's the same process.

-Run the app, it should connect within 5 seconds or so, automatically. If it fails or crashes, try connecting again. If that doesn't work restart SteamVR, or reinstall the plugin, or try connecting with the OVR sample and the same IP address (in a different config file, see OVR dir).

NOTES: 

-If you change your CloudXR resolution or refresh rate / streaming framerate in the config, you must restart SteamVR for the changes to take effect the next time it connects. There is currently no way around this limitation, it's how CloudXR SteamVR plugin was designed.

-Running without SteamVR running at all, should be possible "soon", when Nvidia releases their Monado-based PC side OpenXR runtime which has direct CloudXR server integration. 

------

Funding and further development:

Hi, if you find this software useful and would like to contribute to further development (or need new features added), here are my Github Sponsor and Patreon pages:

https://github.com/sponsors/BattleAxeVR

https://www.patreon.com/posts/ok-cloud-about-100392912?utm_medium=social&utm_source=twitter&utm_campaign=postshare_creator&utm_content=join_link
