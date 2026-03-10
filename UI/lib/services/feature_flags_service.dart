import 'dart:convert';
import 'package:http/http.dart' as http;
import 'package:shared_preferences/shared_preferences.dart';

class FeatureFlagsService {
  // Provide via --dart-define=FEATURE_FLAGS_URL=https://your-domain/flags.json
  static const String _flagsUrl = String.fromEnvironment(
    'FEATURE_FLAGS_URL',
    defaultValue: '',
  );

  static const String _skinCreatorKey = 'skin_creator_enabled';
  static const String _prefsPrefix = 'feature_flag_';

  static Future<bool> isSkinCreatorEnabled() async {
    final prefs = await SharedPreferences.getInstance();
    final cached = prefs.getBool('$_prefsPrefix$_skinCreatorKey');

    // Default to locked if no remote value is available.
    bool enabled = cached ?? false;

    if (_flagsUrl.isEmpty) {
      return enabled;
    }

    try {
      final response = await http
          .get(Uri.parse(_flagsUrl))
          .timeout(const Duration(seconds: 2));
      if (response.statusCode != 200) {
        return enabled;
      }

      final decoded = jsonDecode(response.body);
      if (decoded is! Map) {
        return enabled;
      }

      final dynamic direct = decoded[_skinCreatorKey];
      final dynamic nested =
          (decoded['features'] is Map) ? decoded['features'][_skinCreatorKey] : null;

      final bool? parsed = _toBool(direct) ?? _toBool(nested);
      if (parsed == null) {
        return enabled;
      }

      enabled = parsed;
      await prefs.setBool('$_prefsPrefix$_skinCreatorKey', enabled);
      return enabled;
    } catch (_) {
      return enabled;
    }
  }

  static bool? _toBool(dynamic value) {
    if (value is bool) return value;
    if (value is num) return value != 0;
    if (value is String) {
      final v = value.trim().toLowerCase();
      if (v == 'true' || v == '1' || v == 'yes' || v == 'on') return true;
      if (v == 'false' || v == '0' || v == 'no' || v == 'off') return false;
    }
    return null;
  }
}
