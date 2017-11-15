<?php // no direct access
defined('_JEXEC') or die('Restricted access'); ?>

<!-- This div is for the controls -->
<div id="controls_<?php echo $this->divid; ?>">
</div>

<!-- This div will contain the map -->
<div id="<?php echo $this->divid; ?>" style="<?php echo $this->divstyle; ?>">
</div>

<noscript>
	<b>JavaScript must be enabled in order for you to use Google Maps.</b> 
	However, it seems JavaScript is either disabled or not supported by your browser. 
	To view Google Maps, enable JavaScript by changing your browser options, and then 
	try again.
</noscript>

<script type="text/javascript">
	//<![CDATA[
		<?php echo $this->code; ?>
	//]]>
</script>
