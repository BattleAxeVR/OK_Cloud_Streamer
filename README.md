# OK_Cloud_Streamer
An Open-source CloudXR 4 client, based on OpenXR with OK performance and features.

![OK_Cloud_Streamer](https://github.com/BattleAxeVR/OK_Cloud_Streamer/assets/11604039/89819682-e770-4f56-b47f-4752f4646db0)

("fluffy cloud" generated by https://hotpot.ai/art-generator)

Goals:

-Provide an Open Source OpenXR-based client for CloudXR 4.0

https://developer.nvidia.com/cloudxr-sdk

-Graphics library is Facebook IGL:

https://github.com/BattleAxeVR/igl/tree/ok_cloud_streamer

-Legal / licensing: I was told by Nvidia reps on their forums that it's ok if I share the SteamVR plugin installer and libraries for CXR, as long as I use a permissive license such as MIT, which I am. All rights reserved and credit due to NV for their CloudXR library. NB: Please reach out to me if there are any issues w/ this repo's legality and I will correct them immediately, or even take this entire project down. My relationship with Nvidia matters far more to me than this project.

-Support any and fall Quest (or other) standalone features available over OpenXR, such as eye-tracking, full body tracking simultaneous hands + controller tracking, via the custom non-VR Controller input data and server-side API functionality. 

-Goal is NOT to try and beat Virtual Desktop in terms of overall video quality, as the image quality is limited by CloudXR which I believe is inferior (for now). However CXR should run better over 5G and cloud scenarios than local wifi, due to frame dejittering and other advanced features that are out of scope to discuss here. 

-The direct server-side API can also enable a PC game/app to obviate the need for any OpenXR compositor on the PC side entirely, reducing overhead, synchronization and latency. However the goal is to use Monado or other open source solutions as a truly open source OpenXR runtime on PC that this client can connect to. Such a server will be included in this repo for a full A-to-Z solution.

-The client being open source means you can enable platform-wide features for PC VR gaming, such as waist-oriented locomotion (a software-only version of Decamove). See here:

https://twitter.com/BattleAxeVR/status/1683233951614107648

Full Source + prebuilt APKs/.exe for the waist-loco demo (which will be ported into the client here shortly):

https://github.com/BattleAxeVR/OpenXR-SDK-Source

NOTE: The actual OpenXR implementation isn't pushed yet, I'm still debugging / tweaking it. I pushed the reference Oculus OVR sample client as APKs, that I built as a convenience. There is another README in the directory. The SteamVR Plugin supplied by Nvidia as part of their CloudXR SDK. The sample does not work very reliably, and it has some wavey lines. The final client won't have such obvious visual defects.
