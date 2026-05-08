import 'dart:io';
import 'package:flutter/foundation.dart';

// Registry key where Windows reads autostart entries at user login.
const _regKey = r'HKCU\Software\Microsoft\Windows\CurrentVersion\Run';
const _regValueName = 'ConsoleDeckPro';

class StartupService {
  /// Returns true if Console Deck PRO is registered to start with Windows.
  static Future<bool> isEnabled() async {
    if (!Platform.isWindows) return false;
    try {
      // ProcessStartMode.detached uses DETACHED_PROCESS on Windows, so reg.exe
      // runs without a console window (avoids a CMD flash from a GUI app).
      final process = await Process.start(
        'reg',
        ['query', _regKey, '/v', _regValueName],
        mode: ProcessStartMode.detached,
      );
      return await process.exitCode == 0;
    } catch (e) {
      if (kDebugMode) print('[StartupService] isEnabled error: $e');
      return false;
    }
  }

  /// Adds or removes the autostart registry entry.
  static Future<bool> setEnabled(bool enable) async {
    if (!Platform.isWindows) return false;
    try {
      if (enable) {
        final exePath = Platform.resolvedExecutable;
        // Quote the path so Windows Run key handles spaces in the install dir.
        final process = await Process.start(
          'reg',
          ['add', _regKey, '/v', _regValueName, '/t', 'REG_SZ', '/d', '"$exePath"', '/f'],
          mode: ProcessStartMode.detached,
        );
        return await process.exitCode == 0;
      } else {
        final process = await Process.start(
          'reg',
          ['delete', _regKey, '/v', _regValueName, '/f'],
          mode: ProcessStartMode.detached,
        );
        return await process.exitCode == 0;
      }
    } catch (e) {
      if (kDebugMode) print('[StartupService] setEnabled error: $e');
      return false;
    }
  }
}
