#!/bin/bash

APP_ID="com.example.ble_color_viewer"  # Replace this with your actual app 
ID

echo "🧹 Cleaning..."
flutter clean

echo "📦 Getting dependencies..."
flutter pub get

echo "🏗️ Building APK..."
flutter build apk --release

APK_PATH="build/app/outputs/flutter-apk/app-release.apk"

if [ ! -f "$APK_PATH" ]; then
  echo "❌ APK not found at $APK_PATH"
  exit 1
fi

echo "🔍 Checking if $APP_ID is already installed..."
if adb shell pm list packages | grep -q "$APP_ID"; then
  echo "🧼 Uninstalling existing app..."
  adb uninstall "$APP_ID"
else
  echo "✅ No existing install found."
fi

echo "📲 Installing new APK..."
adb install "$APK_PATH"

echo "✅ Done!"

