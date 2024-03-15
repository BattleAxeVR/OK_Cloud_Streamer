# OK_Cloud_Streamer
An Open-source CloudXR 4 client, based on OpenXR with OK performance and features.

![OK_Cloud_Streamer](https://github.com/BattleAxeVR/OK_Cloud_Streamer/assets/11604039/89819682-e770-4f56-b47f-4752f4646db0)

![image](https://github.com/BattleAxeVR/OK_Cloud_Streamer/assets/11604039/4e429c06-a221-4944-b847-1d788794a9d5)

![image](https://github.com/BattleAxeVR/OK_Cloud_Streamer/assets/11604039/9ac42c49-8ae9-412f-b3e6-c7c82dd78a2f)


Goals:

-Provide an Open Source OpenXR-based client for CloudXR 4.0

https://developer.nvidia.com/cloudxr-sdk

-Graphics library is Facebook IGL:

https://github.com/BattleAxeVR/igl/tree/ok_cloud_streamer

-Legal / licensing: I was told by Nvidia reps on their forums that it's ok if I share the SteamVR plugin installer and libraries for CXR, as long as I use a permissive license such as MIT, which I am. All rights reserved and credit due to NV for their CloudXR library. 

-Support any and fall Quest (or other) standalone features available over OpenXR, such as eye-tracking, full body tracking simultaneous hands + controller tracking, via the custom non-VR Controller input data and server-side API functionality. 

-The direct server-side API can also enable a PC game/app to obviate the need for any OpenXR compositor on the PC side entirely, reducing overhead, synchronization and latency. However the goal is to use Monado or other open source solutions as a truly open source OpenXR runtime on PC that this client can connect to. Such a server will be included in this repo for a full A-to-Z solution.

-The client being open source means you can enable platform-wide features for PC VR gaming, such as waist-oriented locomotion (a software-only version of Decamove). 

See here:

https://twitter.com/BattleAxeVR/status/1683233951614107648

Full Source + prebuilt APKs/.exe for the waist-loco demo (which will be ported into the client here shortly):

https://github.com/BattleAxeVR/OpenXR-SDK-Source

------

Funding:

Hi, I'm working overtime to get this out to the public so you can get full body tracking and eye-tracking and various other features, but I could use some help! Here's my Patreon page, I would appreciate some tips to encourage my work on this project and other VR gaming projects.

https://www.patreon.com/posts/ok-cloud-about-100392912?utm_medium=social&utm_source=twitter&utm_campaign=postshare_creator&utm_content=join_link
