function sendBackToPebble(lat, long, hasData, offsetHrs) {
	// Assemble dictionary using our keys, Minutes need to be flipped in sign
	var dictionary = {
		'KEY_LATITUDE': lat,
		'KEY_LONGITUDE': long,
		'KEY_USEOLDDATA': hasData,
		'KEY_UTCh': offsetHrs,
	};

	// Send to Pebble
	Pebble.sendAppMessage(dictionary, function(e) {
		console.log('Pos info sent to Pebble successfully!');
	},
						  function(e) {
							  console.log('Error sending pos info to Pebble!');
						  }
						 );
}

// Function for when location gathering is successful
function locationSuccess(pos) {
	// Return the position data to the watch
	var latitude = parseInt(pos.coords.latitude * 100);
	var longitude = parseInt(pos.coords.longitude * 100);
	var useOldData = 1;		//Have to set new data to 1 and no data as 0, so the fallback is no data (empty = 0) -> This is for the watch side of things
	
	// Get the number of hours to add to convert time at this position to UTC
	var offsetHrs = new Date().getTimezoneOffset() / -60;
	
	sendBackToPebble(latitude, longitude, useOldData, offsetHrs);
}

// Function for when the location gathering is unsuccessful
function locationError(err) {
	// Return a failure --> Use last know position
	var latitude = 0;
	var longitude = 0;
	var useNewData = 0;
	var offsetHrs = 0;

	sendBackToPebble(latitude, longitude, useNewData, offsetHrs);
}

// Function to get the current position
function getLocation() {
	navigator.geolocation.getCurrentPosition(locationSuccess, locationError, {enableHighAccuracy: false, timeout: 5000, maximumAge: 900000});
}

// Listen for when the watchface is opened
Pebble.addEventListener('ready', function(e) {
	console.log('PebbleKit JS ready!');
}
					   );

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage', function(e) {
	console.log('AppMessage received!');
	//Get Location
	getLocation();
}
					   );