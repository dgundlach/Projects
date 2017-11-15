<?php
class json {

	function isVector (&$array) {

		$next = 0;
		foreach ($array as $k => $v) {
			if ($k != $next) {
				return false;
			}
			$next++;
		}
		return true;
	}

	function JsonValue($value) {

		switch(gettype($value)) {
			case 'double':
			case 'integer':
				return $value;
			case 'bool':
				return $value ? 'true' : 'false';
			case 'string':
				return "'" . addslashes($value) . "'";
			case 'NULL':
				return 'null';
			case 'object':
				return json::JsonEncode($value);
			case 'array':
				if (json::isVector($value)) {
					$a = array();
					foreach ($value as $v) {
						$a[] = json::JsonEncode($v);
					}
					return "[" . implode(',', $a) . "]";
				} else {
					return json::JsonEncode($value);
				}
			default:
				return "'" . addslashes(gettype($value)) . "'";
		}
	}

	function JsonEncode($data) {

		$arr = array();
		foreach ($data as $k => $v) {
			if (gettype($k) == 'integer') {
				$k = 'idx_' . $k;
			}
			$arr[] = "'$k':" . json::JsonValue($v);
		}

		return '{' . implode(',', $arr) . '}';
	}
}
?>
