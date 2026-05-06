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
      final result = await Process.run(
        'reg',
        ['query', _regKey, '/v', _regValueName],
        runInShell: true,
      );
      return result.exitCode == 0;
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
        final result = await Process.run(
          'reg',
          ['add', _regKey, '/v', _regValueName, '/t', 'REG_SZ', '/d', exePath, '/f'],
          runInShell: true,
        );
        return result.exitCode == 0;
      } else {
        final result = await Process.run(
          'reg',
          ['delete', _regKey, '/v', _regValueName, '/f'],
          runInShell: true,
        );
        return result.exitCode == 0;
      }
    } catch (e) {
      if (kDebugMode) print('[StartupService] setEnabled error: $e');
      return false;
    }
  }
}
