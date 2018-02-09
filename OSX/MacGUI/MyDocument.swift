/*
 * (C) 2018 Dirk W. Hoffmann. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

import Foundation

class MyDocument : NSDocument {
    
    //! @brief   Emulator proxy object
    /*! @details This object is an objC object bridging between the GUI (written in Swift)
     *           an the core emulator (written in C++).
     */
    var c64: C64Proxy!
    
    //! @brief   Attachment
    /*! @details When the GUI receives the READY_TO_RUN message from the emulator,
     *           it checks his variable. If an attachment is found, e.g., a T64 file archive,
     *           is displays a user dialog. The user can then choose to mount the archive
     *           as a disk or to flash a single file into memory. If the attachment is a
     *           snapshot, it is read into the emulator without asking the user.
     */
    @objc var attachment: ContainerProxy? = nil
        
    override init() {
        
        NSLog("MyDocument::\(#function)")
        super.init()
        
        // Create emulator instance and try to load ROMs
        c64 = C64Proxy()
        let defaults = UserDefaults.standard
        loadRom(defaults.string(forKey: VC64BasicRomFileKey))
        loadRom(defaults.string(forKey: VC64CharRomFileKey))
        loadRom(defaults.string(forKey: VC64KernelRomFileKey))
        loadRom(defaults.string(forKey: VC64VC1541RomFileKey))
    }
 
    override open func makeWindowControllers() {
        
        NSLog("\(#function)")
        
        let nibName = NSNib.Name(rawValue: "MyDocument")
        let controller = MyController.init(windowNibName: nibName)
        controller.c64 = c64
        self.addWindowController(controller)
    }
    
    //
    // Loading and saving
    //
    
    override open func read(from data: Data, ofType typeName: String) throws {
        
        let size = data.count
        let ptr = (data as NSData).bytes
        
        track("Trying to read \(typeName) file from buffer with \(size) bytes")
        
        switch (typeName) {
        
        case "VC64":
            let snapshot = SnapshotProxy.make(withBuffer: ptr, length: size)
            c64.load(fromSnapshot: snapshot)
            return
            
        case "CRT":
            attachment = CRTProxy.make(withBuffer: ptr, length: size)
            break
        case "TAP":
            attachment = TAPProxy.make(withBuffer: ptr, length: size)
            break
        case "T64":
            attachment = T64Proxy.make(withBuffer: ptr, length: size)
            break
        case "PRG":
            attachment = PRGProxy.make(withBuffer: ptr, length: size)
            break
        case "P00":
            attachment = P00Proxy.make(withBuffer: ptr, length: size)
            break
        case "G64":
            attachment = G64Proxy.make(withBuffer: ptr, length: size)
            break
        case "NIB":
            attachment = NIBProxy.make(withBuffer: ptr, length: size)
            break

        default:
            track("Unknown file type")
            throw NSError(domain: NSOSStatusErrorDomain, code: unimpErr, userInfo: nil)
        }
        
        fileURL = nil // Create an 'Untitled' document
    }
    
    @discardableResult
    @objc func loadRom(_ filename: String?) -> Bool {
        
        if (filename == nil) {
            return false
        }
        
        let defaults = UserDefaults.standard
        
        if c64.loadBasicRom(filename!) {
            NSLog("Basic ROM:  \(filename!)")
            defaults.set(filename, forKey: VC64BasicRomFileKey)
            return true
        }
        if c64.loadCharRom(filename!) {
            NSLog("Char ROM:   \(filename!)")
            defaults.set(filename, forKey: VC64CharRomFileKey)
            return true
        }
        if c64.loadKernelRom(filename!) {
            NSLog("Kernel ROM: \(filename!)")
            defaults.set(filename, forKey: VC64KernelRomFileKey)
            return true
        }
        if c64.loadVC1541Rom(filename!) {
            NSLog("VC1541 ROM: \(filename!)")
            defaults.set(filename, forKey: VC64VC1541RomFileKey)
            return true
        }
        
        return false
    }

    //
    // Saving
    //
    
    override open func data(ofType typeName: String) throws -> Data {
        
        track("Trying to write \(typeName) file.")
        
        if typeName == "VC64" {

            NSLog("Type is VC64")
            
            // Take snapshot
            if let snapshot = SnapshotProxy.make(withC64: c64) {

                // Write to data buffer
                if let data = NSMutableData.init(length: snapshot.sizeOnDisk()) {
                    snapshot.write(toBuffer: data.mutableBytes)
                    return data as Data
                }
            }
        }
        
        throw NSError(domain: NSOSStatusErrorDomain, code: unimpErr, userInfo: nil)
    }
    
    open override func removeWindowController(_ windowController: NSWindowController) {
        
        NSLog("MyDocument:\(#function)")

        super.removeWindowController(windowController)
        
        // Shut down the emulator.
        // Note that all GUI elements need to be inactive when we set the proxy to nil.
        // Hence, the emulator should be shut down as late as possible.
        c64.kill()
    }
    
}
