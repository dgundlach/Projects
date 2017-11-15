<?php

/**
 * Markers Model for GoogleMaps Component
 * 
 * @package    Joomla
 * @subpackage Components
 * @link
 * @license		GNU/GPL
 */

// Check to ensure this file is included in Joomla!
defined('_JEXEC') or die();

jimport( 'joomla.application.component.model' );

class GooglemapsModelMarkers extends Jmodel {

       	/**
	 * Gets the marker
	 * @return marker data
	 */		

	var $_data;
	var $_id;

	function _fetch() {

		if (!empty($this->_data)) {
			return;
		}
		$ctypes = &GooglemapsModelContenttypes::getContenttypes();
		foreach ($ctypes as $type) {
			$ctype = $type->contentType;
			$m = $this->_getList('SELECT m.markerId, m.title, l.locationName, l,address,'
					. ' l.city, l.state, l.zipcode, l.country, l.lat, l.lng, l.alt,'
					. ' c.*, t.contentType, i.*'
					. ' from #__GoogleMaps_Markers m'
					. ' JOIN #__GoogleMaps_Locations l USING(locationId)'
					. ' JOIN #__GoogleMaps_Content_' . $ctype . ' c USING(markerId)'
					. ' JOIN #__GoogleMaps_ContentTypes t USING(contentType)'
					. ' JOIN #__GoogleMaps_Icons i USING(iconId) where mapId=' . $this->_id);
			$this->_data = array_merge((array)$this->_data, (array)$m);
		}
	}

	function &getMarkers($id = 0) {

		$instance = GooglemapsModelMarkers::getInstance($id);
		return $instance->_data;
	}

	function &getInstance($id = 0) {
		static $instances;

		if (!($id = $id ? $id : JRequest::getInt('id', 0))) {
			return null;
		}
		if (!$instances) {
			$instances = array();
		}
		if (!array_key_exists($id, $instances)) {
			$instances[$id] = new GooglemapsModelMarkers;
			$instances[$id]->_id = $id;
			$instances[$id]->_fetch();
		}
		return $instances[$id];
	}
}

?>
