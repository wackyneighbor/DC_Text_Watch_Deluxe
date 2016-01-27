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
Pebble.addEventListener('showConfiguration', function(e) {
	var SinglePrefixType = getStorageValue('SinglePrefixType', '2'); // Show O' before single digits
	var ColorScheme = getStorageValue('ColorScheme', '1'); // Default black color scheme
	
	// Define default values for custom color scheme 1, at time of installation
	var BackgroundColor1 = getStorageValue('BackgroundColor1', 0xFFFFAA); // GColorPastelYellow
	var TextLine1Color1 = getStorageValue('TextLine1Color1', 0x550055); // GColorImperialPurple
	var TextLine2Color1 = getStorageValue('TextLine2Color1', 0xAA5500); // GColorWindsorTan
 	var TextLine3Color1 = getStorageValue('TextLine3Color1', 0x55AA00); // GColorKellyGreen
 	var TextDayColor1 = getStorageValue('TextDayColor1', 0xFF0055); // GColorFolly
 	var TextDateColor1 = getStorageValue('TextDateColor1', 0xFF0055); // GColorFolly
 	var TimeIndicatorColor1 = getStorageValue('TimeIndicatorColor1', 0x55AA00); // GColorKellyGreen
 	var SunriseIndicatorColor1 = getStorageValue('SunriseIndicatorColor1', 0xAA5555); // GColorRoseVale
 	var SunsetIndicatorColor1 = getStorageValue('SunsetIndicatorColor1', 0xAA5555); // GColorRoseVale

	// Define default values for custom color scheme 2, at time of installation
	var BackgroundColor2 = getStorageValue('BackgroundColor2', 0x00000); // GColorBlack
 	var TextLine1Color2 = getStorageValue('TextLine1Color2', 0xAA5500); // GColorWindsorTan
 	var TextLine2Color2 = getStorageValue('TextLine2Color2', 0xAA5500); // GColorWindsorTan
 	var TextLine3Color2 = getStorageValue('TextLine3Color2', 0xAA5500); // GColorWindsorTan
 	var TextDayColor2 = getStorageValue('TextDayColor2', 0xFFFF55); // GColorIcterine
 	var TextDateColor2 = getStorageValue('TextDateColor2', 0xFFFFAA); // GColorPastelYellow
 	var TimeIndicatorColor2 = getStorageValue('TimeIndicatorColor2', 0xFFAA00); // GColorChromeYellow
 	var SunriseIndicatorColor2 = getStorageValue('SunriseIndicatorColor2', 0x555500); // GColorArmyGreen
 	var SunsetIndicatorColor2 = getStorageValue('SunsetIndicatorColor2', 0x555500); // GColorArmyGreen

	// Define default values for custom color scheme 3, at time of installation
	var BackgroundColor3 = getStorageValue('BackgroundColor3', 0xFF0000); // GColorRed
	var TextLine1Color3 = getStorageValue('TextLine1Color3', 0xFFFF00); // GColorYellow
	var TextLine2Color3 = getStorageValue('TextLine2Color3', 0xFFFF55); // GColorIcterine
 	var TextLine3Color3 = getStorageValue('TextLine3Color3', 0xFFFF55); // GColorIcterine
 	var TextDayColor3 = getStorageValue('TextDayColor3', 0xFFFFAA); // GColorPastelYellow
 	var TextDateColor3 = getStorageValue('TextDateColor3', 0xFFFFAA); // GColorPastelYellow
 	var TimeIndicatorColor3 = getStorageValue('TimeIndicatorColor3', 0xAA00FF); // GColorVividViolet
 	var SunriseIndicatorColor3 = getStorageValue('SunriseIndicatorColor3', 0x000000); // GColorBlack
 	var SunsetIndicatorColor3 = getStorageValue('SunsetIndicatorColor3', 0x000000); // GColorBlack
	
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
		

		// fill local variables with the choice of colors to send to the phone
		var BackgroundColor;
		var TextLine1Color;
		var TextLine2Color;
		var TextLine3Color;
		var TextDayColor;
		var TextDateColor;
		var TimeIndicatorColor;
		var SunriseIndicatorColor;
		var SunsetIndicatorColor;
		
			// Setup default color scheme here
		if(settings.ColorScheme == "1"){
			BackgroundColor = 0x000000; // GColorBlack
			TextLine1Color = 0xFFFFFF; // GColorWhite
			TextLine2Color = 0xFFFFFF; // GColorWhite
			TextLine3Color = 0xFFFFFF; // GColorWhite
			TextDayColor = 0xFFFFFF; // GColorWhite
			TextDateColor = 0xFFFFFF; // GColorWhite
			TimeIndicatorColor = 0xAAAA00; // GColorLimerick
			SunriseIndicatorColor = 0xAAAAAA; // GColorLightGray
			SunsetIndicatorColor = 0xAAAAAA;  // GColorLightGray

			// Setup inverted color scheme here
		} else if (settings.ColorScheme == "2"){
			BackgroundColor = 0xFFFFFF; // GColorWhite
			TextLine1Color = 0x000000; // GColorBlack
			TextLine2Color = 0x000000; // GColorBlack
			TextLine3Color = 0x000000; // GColorBlack
			TextDayColor = 0x000000; // GColorBlack
			TextDateColor = 0x000000; // GColorBlack
			TimeIndicatorColor = 0xAAAA00; // GColorLimerick
			SunriseIndicatorColor = 0x555555; // GColorDarkGray
			SunsetIndicatorColor = 0x555555; // GColorDarkGray

			// Setup custom color scheme 1
		} else if (settings.ColorScheme == "3"){
			BackgroundColor = parseInt(settings.BackgroundColor1, 16);
			TextLine1Color = parseInt(settings.TextLine1Color1, 16);
			TextLine2Color = parseInt(settings.TextLine2Color1, 16);
			TextLine3Color = parseInt(settings.TextLine3Color1, 16);
			TextDayColor =  parseInt(settings.TextDayColor1, 16);
			TextDateColor = parseInt(settings.TextDateColor1, 16);
			TimeIndicatorColor = parseInt(settings.TimeIndicatorColor1, 16);
			SunriseIndicatorColor = parseInt(settings.SunriseIndicatorColor1, 16);
			SunsetIndicatorColor =  parseInt(settings.SunriseIndicatorColor1, 16);

			// Setup custom color scheme 2
		} else if (settings.ColorScheme == "4"){
			BackgroundColor = parseInt(settings.BackgroundColor2, 16);
			TextLine1Color = parseInt(settings.TextLine1Color2, 16);
			TextLine2Color = parseInt(settings.TextLine2Color2, 16);
			TextLine3Color = parseInt(settings.TextLine3Color2, 16);
			TextDayColor =  parseInt(settings.TextDayColor2, 16);
			TextDateColor = parseInt(settings.TextDateColor2, 16);
			TimeIndicatorColor = parseInt(settings.TimeIndicatorColor2, 16);
			SunriseIndicatorColor = parseInt(settings.SunriseIndicatorColor2, 16);
			SunsetIndicatorColor =  parseInt(settings.SunriseIndicatorColor2, 16);

			// Setup custom color scheme 3
		} else if (settings.ColorScheme == "5"){
			BackgroundColor = parseInt(settings.BackgroundColor3, 16);
			TextLine1Color = parseInt(settings.TextLine1Color3, 16);
			TextLine2Color = parseInt(settings.TextLine2Color3, 16);
			TextLine3Color = parseInt(settings.TextLine3Color3, 16);
			TextDayColor =  parseInt(settings.TextDayColor3, 16);
			TextDateColor = parseInt(settings.TextDateColor3, 16);
			TimeIndicatorColor = parseInt(settings.TimeIndicatorColor3, 16);
			SunriseIndicatorColor = parseInt(settings.SunriseIndicatorColor3, 16);
			SunsetIndicatorColor =  parseInt(settings.SunriseIndicatorColor3, 16);
			}
		
		Pebble.sendAppMessage({"KEY_SINGLE_PREFIX_TYPE" : parseInt(settings.SinglePrefixType, 10),
							   "KEY_BACKGROUND_COLOR": BackgroundColor,
							   "KEY_TEXT_LINE_1_COLOR": TextLine1Color,
							   "KEY_TEXT_LINE_2_COLOR": TextLine2Color,
							   "KEY_TEXT_LINE_3_COLOR": TextLine3Color,
							   "KEY_TEXT_DAY_COLOR": TextDayColor,
							   "KEY_TEXT_DATE_COLOR": TextDateColor,
							   "KEY_TIME_INDICATOR_COLOR": TimeIndicatorColor,
							   "KEY_SUNRISE_INDICATOR_COLOR": SunriseIndicatorColor,
							   "KEY_SUNSET_INDICATOR_COLOR": SunsetIndicatorColor,
							  });
	}
  });
