/*
 * Copyright (c) 2019-2020, NVIDIA CORPORATION. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

package com.nvidia.cloudxr.ovr;

//import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.content.pm.PackageManager;

import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

public class MainActivity extends android.app.NativeActivity {
    public static final String TAG = "CloudXR";

    private static String cmdlineOptions = "";
    private final int PERMISSION_REQUEST_CODE = 1;

    private static Boolean resumeReady = false;
    private static Boolean permissionDone = false;
    private static Boolean didResume = false;

    static {
        System.loadLibrary("vrapi");
        System.loadLibrary("CloudXRClientOVR");
    }

    static native void nativeHandleLaunchOptions(String jcmdline);

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        // do super first, as that sets up some native things.
        super.onCreate(savedInstanceState);

        // check for any data passed to our activity that we want to handle
        cmdlineOptions = getIntent().getStringExtra("args");

        // check for permission for any 'dangerous' class features.
        // Note that INTERNET is normal and pre-granted, and READ_EXTERNAL is implicitly granted when accepting WRITE.
        if (ContextCompat.checkSelfPermission(this, "android.permission.WRITE_EXTERNAL_STORAGE") != PackageManager.PERMISSION_GRANTED ||
                ContextCompat.checkSelfPermission(this, "android.permission.RECORD_AUDIO") != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[] {
                    "android.permission.WRITE_EXTERNAL_STORAGE",
                    "android.permission.RECORD_AUDIO"
            }, PERMISSION_REQUEST_CODE);
            Log.w(TAG, "Waiting for permissions from user...");
        } else {
            permissionDone = true;
        }

        permissionDone = true;
    }

    protected void doResume() {
        didResume = true;

        // send down to native any runtime options now that we're past permissions...
        nativeHandleLaunchOptions(cmdlineOptions);
    }

    @Override
    protected void onResume() {
        Log.d(TAG, this + " onResume()");
        super.onResume();

        resumeReady = true;
        if (permissionDone && !didResume)
            doResume();
    }

    @Override
    public void onRequestPermissionsResult(int requestCode,  String[] permissions,  int[] grantResults) {
        if (requestCode == PERMISSION_REQUEST_CODE && grantResults != null && grantResults.length > 0) {
            if (grantResults[0] != PackageManager.PERMISSION_GRANTED) {
                Log.e(TAG, "Error: external storage permission has not been granted.  It is required to read launch options file or write logs.");
            }

            if (grantResults[1] != PackageManager.PERMISSION_GRANTED) {
                Log.e(TAG, "Warning: Record audio permission not granted, cannot use microphone.");
            }
        }
        else
        {
            Log.e(TAG, "Bad return for RequestPermissions: ["+requestCode+"] {"+permissions+"} {"+grantResults+"}");
            // TODO: do we need to exit here?
        }

        // we don't currently treat any of these permissions as required/fatal, so continue on...
        permissionDone = true;
        if (!didResume && resumeReady)
            doResume();
    }
}
