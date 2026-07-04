DC's Text Watch Deluxe, for Pebble smartwatches

<i><a href="https://apps.repebble.com/56ab1cfe6ca919990000000e">Pebble app store link</a></i>

<IMG SRC="https://cloud.githubusercontent.com/assets/16750280/12547778/0b42de68-c307-11e5-8310-744512919e15.gif" ALT="Time_Slide.gif" WIDTH=180 HEIGHT=180> --- <IMG SRC="https://cloud.githubusercontent.com/assets/16750280/12535171/8369a7b0-c22f-11e5-948e-c2df00479a72.gif" ALT="Time_Dial.gif" WIDTH=180 HEIGHT=180> --- <IMG SRC="https://cloud.githubusercontent.com/assets/16750280/12535172/89fc4934-c22f-11e5-90aa-ebca46483f1a.gif" ALT="Seasons.gif" WIDTH=180 HEIGHT=180>

<IMG SRC="https://cloud.githubusercontent.com/assets/16750280/12381789/d811c75a-bd45-11e5-9ceb-5e4b993f6339.png" ALT="Screenshot_1_0631_Summer.png" WIDTH=180 HEIGHT=180> --- <IMG SRC="https://cloud.githubusercontent.com/assets/16750280/12413112/4d01b632-be41-11e5-97d2-d9e387b711ea.png" ALT="surf.png" WIDTH=180 HEIGHT=180> --- <IMG SRC="https://cloud.githubusercontent.com/assets/16750280/12413115/4fe2e3ee-be41-11e5-9cf2-3ae9768d1a57.png" ALT="neapolitan.png" WIDTH=180 HEIGHT=180>

This is a variant of Pebble's original "Sliding Text" watchface, with some significant enhancements:
*  Date & day of the week are added at bottom.  On rectangular watches, Month is spelled out in full; on round watches it is abbreviated due to space constraints.  Day of week is fully spelled out.
*  Single digit times can be prefixed with O' (same line), "Oh" (extra line), or neither, at the user's option.  <a href="https://wackyneighbor.github.io/DC_Text_Watch_Deluxe/config.html?ColorScheme=1&SinglePrefixType=2&Enlarge=True&RectTimeAlign=5&RectDateAlign=2&NoGPS=0&BackgroundColor1=FFFFAA&TextLine1Color1=550055&TextLine2Color1=AA5500&TextLine3Color1=55AA00&TextDayColor1=FF0055&TextDateColor1=FF0055&TimeIndicatorColor1=55AA00&SunriseIndicatorColor1=AA5555&SunsetIndicatorColor1=AA5555&BackgroundColor2=000000&TextLine1Color2=AA5500&TextLine2Color2=AA5500&TextLine3Color2=AA5500&TextDayColor2=FFFF55&TextDateColor2=FFFFAA&TimeIndicatorColor2=FFAA00&SunriseIndicatorColor2=555500&SunsetIndicatorColor2=555500&BackgroundColor3=FF0000&TextLine1Color3=FFFF00&TextLine2Color3=FFFF55&TextLine3Color3=FFFF55&TextDayColor3=FFFFAA&TextDateColor3=FFFFAA&TimeIndicatorColor3=00AAAA&SunriseIndicatorColor3=000000&SunsetIndicatorColor3=000000&return_to=https%3A//cloudpebble.repebble.com/ide/emulator/config%3F">You can preview the configuration page here.</a>
*  User can customize the colors, and quickly switch between five saved color schemes.
*  Text justification is configurable.  If user selects a "middle" justification for the time text, there will be a vertical animation when the current time switches between 2 lines and 3 lines.
*  A subtle analog clock feature is incorporated.  A semicircle transverses the perimeter of the screen once a day, with midnight on the bottom and noon on top.  Configuration option to disable this is available, if uncluttered perimeter is preferred.  Also, two dots along the perimeter identify the sunrise and sunset times.  These are computed on the watch, not looked up from a service.  To do this, it must determine location from phone's GPS; dots will not appear until this has completed successfully at least once. Attempts made once a minute until first success, with updates requested each hour, unless disabled by user.
*  Rather than "twelve o'clock", it will say "twelve noon" or "twelve midnight".  On rectangular watches, "midnight" is shown with slightly smaller font, as it is the longest word and would be clipped otherwise.
*  Larger fonts used on the new high-resolution watches, with configuration option to revert to original system fonts if desired.

See <a href="https://github.com/stars/wackyneighbor">projects I've starred</a> for various bits of code I've copied from.

By the way, I don't really know what I'm doing; I'm a mechanical systems designer by trade, not a coder.

<IMG SRC="http://imgs.xkcd.com/comics/code_quality.png" ALT="XKCD #1513 on Code Quality" WIDTH=740 HEIGHT=258>
