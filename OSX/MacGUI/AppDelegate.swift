//
//  AppDelegate.swift
//  VirtualC64
//
//  Created by Dirk Hoffmann on 31.01.18.
//

import Cocoa

@NSApplicationMain
public class AppDelegate: NSObject, NSApplicationDelegate {
    
    public func applicationDidFinishLaunching(_ aNotification: Notification) {
        
        track()
        
        // Make touch bar customizable
        if #available(OSX 10.12.2, *) {
            NSApplication.shared.isAutomaticCustomizeTouchBarMenuItemEnabled = true
        }
    }
    
    public func applicationWillTerminate(_ aNotification: Notification) {

        track()
    }
    
    public func application(_ application: NSApplication, willPresentError error: Error) -> Error {

        track()
        
        let nserror = error as NSError
        
        if (nserror.domain == "VirtualC64") {
            if (nserror.code == 1) {
                return NSError(domain: "", code: 0, userInfo:
                    [NSLocalizedDescriptionKey: "Snapshot from other VirtualC64 release",
                     NSLocalizedRecoverySuggestionErrorKey: "The snapshot was created with a different version of VirtualC64 and cannot be opened."])
            }
        }
        
        return error
    }
    
    /*
    public func applicationShouldHandleReopen(_ sender: NSApplication,
                                              hasVisibleWindows flag: Bool) -> Bool {
        print("\(#function)")
        assert(false)
        return false
    }
    
    public func applicationShouldOpenUntitledFile(_ sender: NSApplication) -> Bool {
        print("\(#function)")
        assert(false)
        return false
    }
 */
}

