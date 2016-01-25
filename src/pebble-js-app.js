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
function getLocation(){
	navigator.geolocation.getCurrentPosition(locationSuccess, locationError, {enableHighAccuracy: false, timeout: 5000, maximumAge: 900000})
	;}

// Function to provide user custom data, or default data otherwise
function getStorageValue(item, default_value){
    var retVal = localStorage.getItem(item);
    //console.log('value' + item + ': ' + String(retVal));
    if (retVal === null || retVal == 'undefined' || retVal == 'null'){
        retVal = default_value;
    }
    return retVal;
}

// Format hex numbers to send to config page properly
function padzeros(num) {
	var s = "000000" + num;
    return s.substr(s.length-6);
}

// Listen for when the watchface is opened
Pebble.addEventListener('ready', function(e) {
	console.log('PebbleKit JS ready!');
});

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage', function(e) {
	console.log('AppMessage received!');
	//Get Location
	getLocation();
});

// Bring up webpage, populated with defaults, or saved user selections
// Defaults are specified here
Pebble.addEventListener('showConfiguration', function(e) {
	var SinglePrefixType = getStorageValue('SinglePrefixType', '2'); // Show O' before single digits
	var ColorScheme = getStorageValue('ColorScheme', '1'); // Default black color scheme
	var BackgroundColor1 = getStorageValue('BackgroundColor1', 0x00AAFF); // GColorVividCerulean
	var TextLine1Color1 = getStorageValue('TextLine1Color1', 0x550000); // GColorBulgarianRose
	var TextLine2Color1 = getStorageValue('TextLine2Color1', 0x555500); // GColorArmyGreen
 	var TextLine3Color1 = getStorageValue('TextLine3Color1', 0x555500); // GColorArmyGreen
 	var TextDayColor1 = getStorageValue('TextDayColor1', 0x5500AA); // GColorIndigo
 	var TextDateColor1 = getStorageValue('TextDateColor1', 0x5500AA); // GColorIndigo
 	var TimeIndicatorColor1 = getStorageValue('TimeIndicatorColor1', 0xFFFFAA); // GColorPastelYellow
 	var SunriseIndicatorColor1 = getStorageValue('SunriseIndicatorColor1', 0x000055); // GColorOxfordBlue
 	var SunsetIndicatorColor1 = getStorageValue('SunsetIndicatorColor1', 0x000055); // GColorOxfordBlue
 	var BackgroundColor2 = getStorageValue('BackgroundColor2', 0xFFAAAA); // GColorMelon
 	var TextLine1Color2 = getStorageValue('TextLine1Color2', 0xAA5500); // GColorWindsorTan
 	var TextLine2Color2 = getStorageValue('TextLine2Color2', 0xFFFFFF); // GColorWhite
 	var TextLine3Color2 = getStorageValue('TextLine3Color2', 0xFF00AA); // GColorFashionMagenta
 	var TextDayColor2 = getStorageValue('TextDayColor2', 0xFFFF00); // GColorYellow
 	var TextDateColor2 = getStorageValue('TextDateColor2', 0x0055FF); // GColorBlueMoon
 	var TimeIndicatorColor2 = getStorageValue('TimeIndicatorColor2', 0x000000); // GColorBlack
 	var SunriseIndicatorColor2 = getStorageValue('SunriseIndicatorColor2', 0xAAFFFF); // GColorCeleste
 	var SunsetIndicatorColor2 = getStorageValue('SunsetIndicatorColor2', 0xAAFFFF); // GColorCeleste
	var BackgroundColor3 = getStorageValue('BackgroundColor3', 0x00AAFF); // GColorVividCerulean
	var TextLine1Color3 = getStorageValue('TextLine1Color3', 0x550000); // GColorBulgarianRose
	var TextLine2Color3 = getStorageValue('TextLine2Color3', 0x555500); // GColorArmyGreen
 	var TextLine3Color3 = getStorageValue('TextLine3Color3', 0x555500); // GColorArmyGreen
 	var TextDayColor3 = getStorageValue('TextDayColor3', 0x5500AA); // GColorIndigo
 	var TextDateColor3 = getStorageValue('TextDateColor3', 0x5500AA); // GColorIndigo
 	var TimeIndicatorColor3 = getStorageValue('TimeIndicatorColor3', 0xFFFFAA); // GColorPastelYellow
 	var SunriseIndicatorColor3 = getStorageValue('SunriseIndicatorColor3', 0x000055); // GColorOxfordBlue
 	var SunsetIndicatorColor3 = getStorageValue('SunsetIndicatorColor3', 0x000055); // GColorOxfordBlue
	
	var URL = 'http://wackyneighbor.github.io/DC_Text_Watch_Deluxe/config.html' +
		'?' +
		'SinglePrefixType=' + encodeURIComponent(SinglePrefixType) + '&' +
		'ColorScheme=' + encodeURIComponent(ColorScheme) + '&' +
		'BackgroundColor1=' + encodeURIComponent(padzeros(BackgroundColor1.toString(16).toUpperCase())) + '&' +
		'TextLine1Color1=' + encodeURIComponent(padzeros(TextLine1Color1.toString(16).toUpperCase())) + '&' +
		'TextLine2Color1=' + encodeURIComponent(padzeros(TextLine2Color1.toString(16).toUpperCase())) + '&' +
		'TextLine3Color1=' + encodeURIComponent(padzeros(TextLine3Color1.toString(16).toUpperCase())) + '&' +
		'TextDayColor1=' + encodeURIComponent(padzeros(TextDayColor1.toString(16).toUpperCase())) + '&' +
		'TextDateColor1=' + encodeURIComponent(padzeros(TextDateColor1.toString(16).toUpperCase())) + '&' +
		'TimeIndicatorColor1=' + encodeURIComponent(padzeros(TimeIndicatorColor1.toString(16).toUpperCase())) + '&' +
		'SunriseIndicatorColor1=' + encodeURIComponent(padzeros(SunriseIndicatorColor1.toString(16).toUpperCase())) + '&' +
		'SunsetIndicatorColor1=' + encodeURIComponent(padzeros(SunsetIndicatorColor1.toString(16).toUpperCase())) + '&' +
		'BackgroundColor2=' + encodeURIComponent(padzeros(BackgroundColor2.toString(16).toUpperCase())) + '&' +
		'TextLine1Color2=' + encodeURIComponent(padzeros(TextLine1Color2.toString(16).toUpperCase())) + '&' +
		'TextLine2Color2=' + encodeURIComponent(padzeros(TextLine2Color2.toString(16).toUpperCase())) + '&' +
		'TextLine3Color2=' + encodeURIComponent(padzeros(TextLine3Color2.toString(16).toUpperCase())) + '&' +
		'TextDayColor2=' + encodeURIComponent(padzeros(TextDayColor2.toString(16).toUpperCase())) + '&' +
		'TextDateColor2=' + encodeURIComponent(padzeros(TextDateColor2.toString(16).toUpperCase())) + '&' +
		'TimeIndicatorColor2=' + encodeURIComponent(padzeros(TimeIndicatorColor2.toString(16).toUpperCase())) + '&' +
		'SunriseIndicatorColor2=' + encodeURIComponent(padzeros(SunriseIndicatorColor2.toString(16).toUpperCase())) + '&' +
		'SunsetIndicatorColor2=' + encodeURIComponent(padzeros(SunsetIndicatorColor2.toString(16).toUpperCase())) + '&' +
		'BackgroundColor3=' + encodeURIComponent(padzeros(BackgroundColor3.toString(16).toUpperCase())) + '&' +
		'TextLine1Color3=' + encodeURIComponent(padzeros(TextLine1Color3.toString(16).toUpperCase())) + '&' +
		'TextLine2Color3=' + encodeURIComponent(padzeros(TextLine2Color3.toString(16).toUpperCase())) + '&' +
		'TextLine3Color3=' + encodeURIComponent(padzeros(TextLine3Color3.toString(16).toUpperCase())) + '&' +
		'TextDayColor3=' + encodeURIComponent(padzeros(TextDayColor3.toString(16).toUpperCase())) + '&' +
		'TextDateColor3=' + encodeURIComponent(padzeros(TextDateColor3.toString(16).toUpperCase())) + '&' +
		'TimeIndicatorColor3=' + encodeURIComponent(padzeros(TimeIndicatorColor3.toString(16).toUpperCase())) + '&' +
		'SunriseIndicatorColor3=' + encodeURIComponent(padzeros(SunriseIndicatorColor3.toString(16).toUpperCase())) + '&' +
		'SunsetIndicatorColor3=' + encodeURIComponent(padzeros(SunsetIndicatorColor3.toString(16).toUpperCase()))	
	;
  console.log('Configuration window opened. ' + URL);
  Pebble.openURL(URL);
});

// Take values back from config page, and send appropriate values back to watch
Pebble.addEventListener("webviewclosed", function(e) {
	if (e.response !== '') {
		console.log('response: ' + decodeURIComponent(e.response));
		
		//Get JSON dictionary
		var settings = JSON.parse(decodeURIComponent(e.response));
		console.log(settings);
	
		// save new user selections so they can be shown on config page next time
		localStorage.setItem('SinglePrefixType', settings.SinglePrefixType);
		localStorage.setItem('ColorScheme', settings.ColorScheme);
		localStorage.setItem('BackgroundColor1', settings.BackgroundColor1);
		localStorage.setItem('TextLine1Color1', settings.TextLine1Color1);
		localStorage.setItem('TextLine2Color1', settings.TextLine2Color1);
		localStorage.setItem('TextLine3Color1', settings.TextLine3Color1);
		localStorage.setItem('TextDayColor1', settings.TextDayColor1);
		localStorage.setItem('TextDateColor1', settings.TextDateColor1);
		localStorage.setItem('TimeIndicatorColor1', settings.TimeIndicatorColor1);
		localStorage.setItem('SunriseIndicatorColor1', settings.SunriseIndicatorColor1);
		localStorage.setItem('SunsetIndicatorColor1', settings.SunsetIndicatorColor1);
		localStorage.setItem('BackgroundColor2', settings.BackgroundColor2);
		localStorage.setItem('TextLine1Color2', settings.TextLine1Color2);
		localStorage.setItem('TextLine2Color2', settings.TextLine2Color2);
		localStorage.setItem('TextLine3Color2', settings.TextLine3Color2);
		localStorage.setItem('TextDayColor2', settings.TextDayColor2);
		localStorage.setItem('TextDateColor2', settings.TextDateColor2);
		localStorage.setItem('TimeIndicatorColor2', settings.TimeIndicatorColor2);
		localStorage.setItem('SunriseIndicatorColor2', settings.SunriseIndicatorColor2);
		localStorage.setItem('SunsetIndicatorColor2', settings.SunsetIndicatorColor2);
		localStorage.setItem('BackgroundColor3', settings.BackgroundColor3);
		localStorage.setItem('TextLine1Color3', settings.TextLine1Color3);
		localStorage.setItem('TextLine2Color3', settings.TextLine2Color3);
		localStorage.setItem('TextLine3Color3', settings.TextLine3Color3);
		localStorage.setItem('TextDayColor3', settings.TextDayColor3);
		localStorage.setItem('TextDateColor3', settings.TextDateColor3);
		localStorage.setItem('TimeIndicatorColor3', settings.TimeIndicatorColor3);
		localStorage.setItem('SunriseIndicatorColor3', settings.SunriseIndicatorColor3);
		localStorage.setItem('SunsetIndicatorColor3', settings.SunsetIndicatorColor3);
		
		Pebble.sendAppMessage({"KEY_SINGLE_PREFIX_TYPE" : parseInt(settings.SinglePrefixType, 10),
							   "KEY_COLOR_SCHEME" : parseInt(settings.ColorScheme, 10)});
		
		// Setup default color scheme here
		if(settings.ColorScheme == "1"){
			Pebble.sendAppMessage({
				"KEY_BACKGROUND_COLOR": 0x000000, // GColorBlack
				"KEY_TEXT_LINE_1_COLOR": 0xFFFFFF, // GColorWhite
				"KEY_TEXT_LINE_2_COLOR": 0xFFFFFF, // GColorWhite
				"KEY_TEXT_LINE_3_COLOR": 0xFFFFFF, // GColorWhite
				"KEY_TEXT_DAY_COLOR": 0xFFFFFF, // GColorWhite
				"KEY_TEXT_DATE_COLOR": 0xFFFFFF, // GColorWhite
				"KEY_TIME_INDICATOR_COLOR": 0xAAAA00, // GColorLimerick
				"KEY_SUNRISE_INDICATOR_COLOR": 0xAAAAAA, // GColorLightGray
				"KEY_SUNSET_INDICATOR_COLOR": 0xAAAAAA // GColorLightGray
			},
								  function(e) {
				console.log("Sending settings data...");
			},
								  function(e) {
				console.log("Settings feedback failed!");
			}
								 );}

		// Setup inverted color scheme here
		else if(settings.ColorScheme == "2"){
			Pebble.sendAppMessage({
			  "KEY_BACKGROUND_COLOR": 0xFFFFFF, // GColorWhite
			  "KEY_TEXT_LINE_1_COLOR": 0x000000, // GColorBlack
			  "KEY_TEXT_LINE_2_COLOR": 0x000000, // GColorBlack
			  "KEY_TEXT_LINE_3_COLOR": 0x000000, // GColorBlack
			  "KEY_TEXT_DAY_COLOR": 0x000000, // GColorBlack
			  "KEY_TEXT_DATE_COLOR": 0x000000, // GColorBlack
			  "KEY_TIME_INDICATOR_COLOR": 0xAAAA00, // GColorLimerick
			  "KEY_SUNRISE_INDICATOR_COLOR": 0x555555, // GColorDarkGray
			  "KEY_SUNSET_INDICATOR_COLOR": 0x555555 // GColorDarkGray
			},
								  function(e) {
				console.log("Sending settings data...");
			},
								  function(e) {
				console.log("Settings feedback failed!");
			}
								 );
		}	

		// Setup custom color scheme 1
		else if(settings.ColorScheme == "3"){
			Pebble.sendAppMessage({
			  "KEY_BACKGROUND_COLOR": parseInt(settings.BackgroundColor1, 16),
			  "KEY_TEXT_LINE_1_COLOR": parseInt(settings.TextLine1Color1, 16),
			  "KEY_TEXT_LINE_2_COLOR": parseInt(settings.TextLine2Color1, 16),
			  "KEY_TEXT_LINE_3_COLOR": parseInt(settings.TextLine3Color1, 16),
			  "KEY_TEXT_DAY_COLOR": parseInt(settings.TextDayColor1, 16),
			  "KEY_TEXT_DATE_COLOR": parseInt(settings.TextDateColor1, 16),
			  "KEY_TIME_INDICATOR_COLOR": parseInt(settings.TimeIndicatorColor1, 16),
			  "KEY_SUNRISE_INDICATOR_COLOR": parseInt(settings.SunriseIndicatorColor1, 16),
			  "KEY_SUNSET_INDICATOR_COLOR": parseInt(settings.SunsetIndicatorColor1, 16)
			},
								  function(e) {
				console.log("Sending settings data...");
			},
								  function(e) {
				console.log("Settings feedback failed!");
			}
								 );
		}	
				
		// Setup custom color scheme 2
		else if(settings.ColorScheme == "4"){
			Pebble.sendAppMessage({
			  "KEY_BACKGROUND_COLOR": parseInt(settings.BackgroundColor2, 16),
			  "KEY_TEXT_LINE_1_COLOR": parseInt(settings.TextLine1Color2, 16),
			  "KEY_TEXT_LINE_2_COLOR": parseInt(settings.TextLine2Color2, 16),
			  "KEY_TEXT_LINE_3_COLOR": parseInt(settings.TextLine3Color2, 16),
			  "KEY_TEXT_DAY_COLOR": parseInt(settings.TextDayColor2, 16),
			  "KEY_TEXT_DATE_COLOR": parseInt(settings.TextDateColor2, 16),
			  "KEY_TIME_INDICATOR_COLOR": parseInt(settings.TimeIndicatorColor2, 16),
			  "KEY_SUNRISE_INDICATOR_COLOR": parseInt(settings.SunriseIndicatorColor2, 16),
			  "KEY_SUNSET_INDICATOR_COLOR": parseInt(settings.SunsetIndicatorColor2, 16)
			},
								  function(e) {
				console.log("Sending settings data...");
			},
								  function(e) {
				console.log("Settings feedback failed!");
			}
								 );
		}
		
		// Setup custom color scheme 3
		else if(settings.ColorScheme == "5"){
			Pebble.sendAppMessage({
			  "KEY_BACKGROUND_COLOR": parseInt(settings.BackgroundColor3, 16),
			  "KEY_TEXT_LINE_1_COLOR": parseInt(settings.TextLine1Color3, 16),
			  "KEY_TEXT_LINE_2_COLOR": parseInt(settings.TextLine2Color3, 16),
			  "KEY_TEXT_LINE_3_COLOR": parseInt(settings.TextLine3Color3, 16),
			  "KEY_TEXT_DAY_COLOR": parseInt(settings.TextDayColor3, 16),
			  "KEY_TEXT_DATE_COLOR": parseInt(settings.TextDateColor3, 16),
			  "KEY_TIME_INDICATOR_COLOR": parseInt(settings.TimeIndicatorColor3, 16),
			  "KEY_SUNRISE_INDICATOR_COLOR": parseInt(settings.SunriseIndicatorColor3, 16),
			  "KEY_SUNSET_INDICATOR_COLOR": parseInt(settings.SunsetIndicatorColor3, 16)
			},
								  function(e) {
				console.log("Sending settings data...");
			},
								  function(e) {
				console.log("Settings feedback failed!");
			}
								 );
		}
	}
  });
