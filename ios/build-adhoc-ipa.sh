#!/bin/bash
security unlock-keychain -p ${!KEYCHAIN_PASSWORD}
xcodebuild -scheme laidoff archive -archivePath build/laidoff
xcodebuild -exportArchive -exportOptionsPlist exportoptions.plist -archivePath "build/laidoff.xcarchive" -exportPath "build/laidoff.ipa"

