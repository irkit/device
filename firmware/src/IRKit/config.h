/*
 Copyright (C) 2013-2014 Masakazu Ohtsuka
  
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 2 of the License, or
 (at your option) any later version.
  
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
  
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef __CONFIG_H__
#define __CONFIG_H__

/*
 * Exposes some general options for your IRKit device.
 */
namespace config {

    // Different modes for the LED feedback.
    typedef enum {
        LED_VERBOSE    = 0, // LED almost always-on to indicate the device status.
        LED_QUIET      = 1, // LED on during setup and blinks when receiving/sending commands. Off when the device is idle.
        LED_SETUP_ONLY = 2, // LED on only during the setup, then always-off.
        LED_OFF        = 3  // /!\ This disables the LED completely.
    } LedFeedbackProfile;

    // Defines what kind of LED profile you want.
    const LedFeedbackProfile ledFeedback = LED_QUIET;

    // Enables/disables cloud-control through the "deviceapi.getirkit.com" server.
    // If you use your IRKit device exclusively over your LAN, you can disable
    // this option: the device won't send regular polling requests to the cloud server.
    // This also lets you setup your device without internet access (no need for a valid device key).
    const bool useCloudControl = true;

}

#endif // __CONFIG_H__


