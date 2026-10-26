<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE eagle SYSTEM "eagle.dtd">
<eagle version="9.6.2">
<drawing>
<settings>
<setting alwaysvectorfont="no"/>
<setting verticaltext="up"/>
</settings>
<grid distance="0.1" unitdist="inch" unit="inch" style="lines" multiple="1" display="yes" altdistance="0.01" altunitdist="inch" altunit="inch"/>
<layers>
<layer number="91" name="Nets" color="2" fill="1" visible="yes" active="yes"/>
<layer number="92" name="Busses" color="1" fill="1" visible="yes" active="yes"/>
<layer number="93" name="Pins" color="2" fill="1" visible="yes" active="yes"/>
<layer number="94" name="Symbols" color="4" fill="1" visible="yes" active="yes"/>
<layer number="95" name="Names" color="7" fill="1" visible="yes" active="yes"/>
<layer number="96" name="Values" color="7" fill="1" visible="yes" active="yes"/>
<layer number="97" name="Info" color="7" fill="1" visible="yes" active="yes"/>
<layer number="1" name="Plus" color="4" fill="1" visible="yes" active="yes"/>
<layer number="102" name="Minus" color="0" fill="1" visible="yes" active="yes"/>
</layers>
<schematic>
<libraries>
<library name="airctl">
<description>AirUpDown system symbols</description>
<symbols>

<symbol name="VCC">
<pin name="VCC" x="0" y="0" visible="off" length="short" direction="sup" rot="R270"/>
<wire x1="-1.778" y1="2.54" x2="1.778" y2="2.54" width="0.254" layer="94"/>
<wire x1="-1.016" y1="3.556" x2="1.016" y2="3.556" width="0.254" layer="94"/>
<wire x1="-0.4" y1="4.572" x2="0.4" y2="4.572" width="0.254" layer="94"/>
<wire x1="0" y1="0" x2="0" y2="2.54" width="0.1524" layer="94"/>
<text x="2.2" y="2.8" size="1.778" layer="96" align="bottom-left">&gt;VALUE</text>
</symbol>
<symbol name="GND">
<pin name="GND" x="0" y="0" visible="off" length="short" direction="sup" rot="R90"/>
<wire x1="0" y1="0" x2="0" y2="-1.27" width="0.1524" layer="94"/>
<wire x1="-2.032" y1="-1.27" x2="2.032" y2="-1.27" width="0.254" layer="94"/>
<wire x1="-1.27" y1="-2.032" x2="1.27" y2="-2.032" width="0.2" layer="94"/>
<wire x1="-0.5" y1="-2.794" x2="0.5" y2="-2.794" width="0.1524" layer="94"/>
</symbol>
<symbol name="FUSE">
<pin name="1" x="-5.08" y="0" visible="pin" length="short" direction="pas" rot="R0"/>
<pin name="2" x="5.08" y="0" visible="pin" length="short" direction="pas" rot="R180"/>
<text x="0" y="2.4" size="1.778" layer="95" align="bottom-center">&gt;NAME</text>
<text x="0" y="-3.6" size="1.778" layer="96" align="bottom-center">&gt;VALUE</text>
<wire x1="-2.54" y1="-1.27" x2="2.54" y2="-1.27" width="0.1524" layer="94"/>
<wire x1="2.54" y1="-1.27" x2="2.54" y2="1.27" width="0.1524" layer="94"/>
<wire x1="2.54" y1="1.27" x2="-2.54" y2="1.27" width="0.1524" layer="94"/>
<wire x1="-2.54" y1="1.27" x2="-2.54" y2="-1.27" width="0.1524" layer="94"/>
<wire x1="-5.08" y1="0" x2="-2.54" y2="0" width="0.1524" layer="94"/>
<wire x1="2.54" y1="0" x2="5.08" y2="0" width="0.1524" layer="94"/>
<wire x1="-1.2" y1="-0.6" x2="1.2" y2="0.6" width="0.1524" layer="94"/>
</symbol>
<symbol name="D">
<pin name="A" x="-5.08" y="0" visible="pin" length="short" direction="pas" rot="R0"/>
<pin name="K" x="5.08" y="0" visible="pin" length="short" direction="pas" rot="R180"/>
<text x="0" y="2.2" size="1.778" layer="95" align="bottom-center">&gt;NAME</text>
<text x="0" y="-3.3" size="1.778" layer="96" align="bottom-center">&gt;VALUE</text>
<wire x1="-2.54" y1="0" x2="-2.54" y2="1.524" width="0.1524" layer="94"/>
<wire x1="-2.54" y1="1.524" x2="0" y2="0" width="0.1524" layer="94"/>
<wire x1="0" y1="0" x2="-2.54" y2="-1.524" width="0.1524" layer="94"/>
<wire x1="-2.54" y1="-1.524" x2="-2.54" y2="0" width="0.1524" layer="94"/>
<wire x1="0" y1="-1.778" x2="0" y2="1.778" width="0.3" layer="94"/>
<wire x1="-5.08" y1="0" x2="-2.54" y2="0" width="0.1524" layer="94"/>
<wire x1="0" y1="0" x2="5.08" y2="0" width="0.1524" layer="94"/>
</symbol>
<symbol name="R">
<pin name="1" x="-5.08" y="0" visible="pin" length="short" direction="pas" rot="R0"/>
<pin name="2" x="5.08" y="0" visible="pin" length="short" direction="pas" rot="R180"/>
<text x="0" y="1.778" size="1.778" layer="95" align="bottom-center">&gt;NAME</text>
<text x="0" y="-3.302" size="1.778" layer="96" align="bottom-center">&gt;VALUE</text>
<wire x1="-2.54" y1="0" x2="-1.905" y2="0" width="0.1524" layer="94"/>
<wire x1="-1.905" y1="0" x2="-1.27" y2="1.016" width="0.1524" layer="94"/>
<wire x1="-1.27" y1="1.016" x2="-0.508" y2="-1.016" width="0.1524" layer="94"/>
<wire x1="-0.508" y1="-1.016" x2="0.254" y2="1.016" width="0.1524" layer="94"/>
<wire x1="0.254" y1="1.016" x2="1.016" y2="-1.016" width="0.1524" layer="94"/>
<wire x1="1.016" y1="-1.016" x2="1.778" y2="1.016" width="0.1524" layer="94"/>
<wire x1="1.778" y1="1.016" x2="2.54" y2="0" width="0.1524" layer="94"/>
<wire x1="2.54" y1="0" x2="5.08" y2="0" width="0.1524" layer="94"/>
<wire x1="-5.08" y1="0" x2="-2.54" y2="0" width="0.1524" layer="94"/>
</symbol>
<symbol name="C">
<pin name="1" x="0" y="5.08" visible="pin" length="short" direction="pas" rot="R90"/>
<pin name="2" x="0" y="-5.08" visible="pin" length="short" direction="pas" rot="R270"/>
<text x="2.0" y="1.0" size="1.778" layer="95" align="bottom-left">&gt;NAME</text>
<text x="2.0" y="-2.2" size="1.778" layer="96" align="bottom-left">&gt;VALUE</text>
<wire x1="-1.778" y1="1.27" x2="1.778" y2="1.27" width="0.254" layer="94"/>
<wire x1="-1.778" y1="-1.27" x2="1.778" y2="-1.27" width="0.254" layer="94"/>
<wire x1="0" y1="5.08" x2="0" y2="1.27" width="0.1524" layer="94"/>
<wire x1="0" y1="-1.27" x2="0" y2="-5.08" width="0.1524" layer="94"/>
</symbol>
<symbol name="VALVE">
<pin name="+" x="-7.62" y="2.54" visible="pin" length="short" direction="pwr" rot="R0"/>
<pin name="-" x="-7.62" y="-2.54" visible="pin" length="short" direction="pwr" rot="R0"/>
<text x="8" y="17" size="1.778" layer="95" align="bottom-center">&gt;NAME</text>
<text x="8" y="-11" size="1.4" layer="96" align="bottom-center">&gt;VALUE</text>
<wire x1="-2.54" y1="-6" x2="8" y2="-6" width="0.3" layer="94"/>
<wire x1="8" y1="-6" x2="8" y2="6" width="0.3" layer="94"/>
<wire x1="8" y1="6" x2="-2.54" y2="6" width="0.3" layer="94"/>
<wire x1="-2.54" y1="6" x2="-2.54" y2="-6" width="0.3" layer="94"/>
<wire x1="-7.62" y1="2.54" x2="-2.54" y2="2.54" width="0.1524" layer="94"/>
<wire x1="-7.62" y1="-2.54" x2="-2.54" y2="-2.54" width="0.1524" layer="94"/>
<text x="-2" y="1.2" size="1.16" layer="94">coil</text>
<wire x1="8" y1="-6" x2="20" y2="-6" width="1.4" layer="92"/>
<wire x1="20" y1="-6" x2="20" y2="6" width="1.4" layer="92"/>
<wire x1="20" y1="6" x2="8" y2="6" width="1.4" layer="92"/>
<wire x1="11" y1="0" x2="15" y2="3.2" width="0.22" layer="94"/>
<wire x1="15" y1="3.2" x2="18" y2="0" width="0.22" layer="94"/>
<wire x1="15" y1="-3.2" x2="18" y2="0" width="0.22" layer="94"/>
<wire x1="14" y1="6" x2="14" y2="14" width="1.6" layer="92"/>
<text x="15.2" y="9" size="1.15" layer="94">air in</text>
<wire x1="20" y1="0" x2="28" y2="0" width="1.6" layer="92"/>
<text x="20.2" y="-5" size="1.15" layer="94">air out</text>
</symbol>
<symbol name="SENSOR">
<pin name="VCC" x="-10.16" y="5.08" visible="pin" length="short" direction="pwr" rot="R0"/>
<pin name="GND" x="-10.16" y="-5.08" visible="pin" length="short" direction="pwr" rot="R0"/>
<pin name="OUT" x="10.16" y="0" visible="pin" length="short" direction="out" rot="R180"/>
<text x="0" y="8.4" size="1.778" layer="95" align="bottom-center">&gt;NAME</text>
<text x="0" y="-9.0" size="1.778" layer="96" align="bottom-center">&gt;VALUE</text>
<wire x1="-5.08" y1="-7.62" x2="5.08" y2="-7.62" width="0.254" layer="94"/>
<wire x1="5.08" y1="-7.62" x2="5.08" y2="7.62" width="0.254" layer="94"/>
<wire x1="5.08" y1="7.62" x2="-5.08" y2="7.62" width="0.254" layer="94"/>
<wire x1="-5.08" y1="7.62" x2="-5.08" y2="-7.62" width="0.254" layer="94"/>
<wire x1="-10.16" y1="5.08" x2="-5.08" y2="5.08" width="0.1524" layer="94"/>
<wire x1="-10.16" y1="-5.08" x2="-5.08" y2="-5.08" width="0.1524" layer="94"/>
<wire x1="5.08" y1="0" x2="10.16" y2="0" width="0.1524" layer="94"/>
<wire x1="0" y1="-7.62" x2="0" y2="-16" width="1.6" layer="92"/>
<text x="1.4" y="-14" size="1.2" layer="94">air</text>
</symbol>
<symbol name="BOARD">
<pin name="VCC" x="-27.94" y="15.24" visible="pin" length="short" direction="pwr" rot="R0"/>
<pin name="GND" x="-27.94" y="5.08" visible="pin" length="short" direction="pwr" rot="R0"/>
<pin name="5V" x="-27.94" y="-5.08" visible="pin" length="short" direction="pwr" rot="R0"/>
<pin name="COM1" x="27.94" y="17.78" visible="pin" length="short" direction="pas" rot="R180"/>
<pin name="NO1" x="27.94" y="10.16" visible="pin" length="short" direction="pas" rot="R180"/>
<pin name="NC1" x="27.94" y="2.54" visible="pin" length="short" direction="nc" rot="R180"/>
<pin name="COM2" x="27.94" y="-7.62" visible="pin" length="short" direction="pas" rot="R180"/>
<pin name="NO2" x="27.94" y="-15.24" visible="pin" length="short" direction="pas" rot="R180"/>
<pin name="NC2" x="27.94" y="-22.86" visible="pin" length="short" direction="nc" rot="R180"/>
<pin name="IO34" x="10.16" y="-33.02" visible="pin" length="short" direction="in" rot="R270"/>
<text x="0" y="26" size="1.778" layer="95" align="bottom-center">&gt;NAME</text>
<text x="0" y="21" size="1.778" layer="96" align="bottom-center">&gt;VALUE</text>
<wire x1="-22.86" y1="-27.94" x2="22.86" y2="-27.94" width="0.254" layer="94"/>
<wire x1="22.86" y1="-27.94" x2="22.86" y2="22.86" width="0.254" layer="94"/>
<wire x1="22.86" y1="22.86" x2="-22.86" y2="22.86" width="0.254" layer="94"/>
<wire x1="-22.86" y1="22.86" x2="-22.86" y2="-27.94" width="0.254" layer="94"/>
<wire x1="-27.94" y1="15.24" x2="-22.86" y2="15.24" width="0.1524" layer="94"/>
<wire x1="-27.94" y1="5.08" x2="-22.86" y2="5.08" width="0.1524" layer="94"/>
<wire x1="-27.94" y1="-5.08" x2="-22.86" y2="-5.08" width="0.1524" layer="94"/>
<wire x1="22.86" y1="17.78" x2="27.94" y2="17.78" width="0.1524" layer="94"/>
<wire x1="22.86" y1="10.16" x2="27.94" y2="10.16" width="0.1524" layer="94"/>
<wire x1="22.86" y1="2.54" x2="27.94" y2="2.54" width="0.1524" layer="94"/>
<wire x1="22.86" y1="-7.62" x2="27.94" y2="-7.62" width="0.1524" layer="94"/>
<wire x1="22.86" y1="-15.24" x2="27.94" y2="-15.24" width="0.1524" layer="94"/>
<wire x1="22.86" y1="-22.86" x2="27.94" y2="-22.86" width="0.1524" layer="94"/>
<wire x1="10.16" y1="-27.94" x2="10.16" y2="-33.02" width="0.1524" layer="94"/>
<text x="-21" y="14" size="1.27" layer="94">VCC 12V</text>
<text x="-21" y="3.8" size="1.27" layer="94">GND</text>
<text x="-21" y="-6.4" size="1.27" layer="94">5V out</text>
<text x="4" y="16.5" size="1.27" layer="94">COM1 GPIO16</text>
<text x="4" y="8.9" size="1.27" layer="94">NO1 inflate</text>
<text x="4" y="1.2" size="1.27" layer="94">NC1</text>
<text x="4" y="-8.9" size="1.27" layer="94">COM2 GPIO17</text>
<text x="4" y="-16.5" size="1.27" layer="94">NO2 deflate</text>
<text x="4" y="-24.1" size="1.27" layer="94">NC2</text>
<text x="11" y="-31" size="1.27" layer="94">IO34</text>
</symbol>


<symbol name="TANK">
<text x="0" y="10" size="1.778" layer="95" align="bottom-center">&gt;NAME</text>
<text x="0" y="-12" size="1.6" layer="96" align="bottom-center">&gt;VALUE</text>
<wire x1="-14" y1="-8" x2="10" y2="-8" width="0.4" layer="94"/>
<wire x1="10" y1="-8" x2="10" y2="8" width="0.4" layer="94"/>
<wire x1="10" y1="8" x2="-14" y2="8" width="0.4" layer="94"/>
<wire x1="-14" y1="8" x2="-14" y2="-8" width="0.4" layer="94"/>
<wire x1="-10" y1="-8" x2="-10" y2="8" width="0.2" layer="94"/>
<wire x1="-6" y1="-8" x2="-6" y2="8" width="0.15" layer="94"/>
<wire x1="10" y1="0" x2="16" y2="0" width="1.6" layer="92"/>
</symbol>
<symbol name="AIRVALVE">
<text x="0" y="8" size="1.6" layer="95" align="bottom-center">&gt;NAME</text>
<text x="0" y="-10" size="1.4" layer="96" align="bottom-center">&gt;VALUE</text>
<wire x1="-6" y1="-5" x2="6" y2="-5" width="0.35" layer="94"/>
<wire x1="6" y1="-5" x2="6" y2="5" width="0.35" layer="94"/>
<wire x1="6" y1="5" x2="-6" y2="5" width="0.35" layer="94"/>
<wire x1="-6" y1="5" x2="-6" y2="-5" width="0.35" layer="94"/>
<wire x1="-4" y1="0" x2="0" y2="3" width="0.25" layer="94"/>
<wire x1="0" y1="3" x2="4" y2="0" width="0.25" layer="94"/>
<wire x1="0" y1="-3" x2="4" y2="0" width="0.25" layer="94"/>
<wire x1="-14" y1="0" x2="-6" y2="0" width="0.5" layer="94"/>
<wire x1="6" y1="0" x2="14" y2="0" width="0.5" layer="94"/>
</symbol>
<symbol name="MANIFOLD">
<text x="4" y="28" size="1.6" layer="95" align="bottom-center">&gt;NAME</text>
<text x="4" y="-30" size="1.4" layer="96" align="bottom-center">&gt;VALUE</text>
<wire x1="-10" y1="-22" x2="18" y2="-22" width="0.4" layer="94"/>
<wire x1="18" y1="-22" x2="18" y2="22" width="0.4" layer="94"/>
<wire x1="18" y1="22" x2="-10" y2="22" width="0.4" layer="94"/>
<wire x1="-10" y1="22" x2="-10" y2="-22" width="0.4" layer="94"/>
<wire x1="-18" y1="16" x2="-10" y2="16" width="1.6" layer="92"/>
<wire x1="0" y1="-22" x2="0" y2="-30" width="1.6" layer="92"/>
<wire x1="18" y1="15" x2="26" y2="15" width="1.6" layer="92"/>
<wire x1="18" y1="5" x2="26" y2="5" width="1.6" layer="92"/>
<wire x1="18" y1="-5" x2="26" y2="-5" width="1.6" layer="92"/>
<wire x1="18" y1="-15" x2="26" y2="-15" width="1.6" layer="92"/>
<wire x1="8" y1="22" x2="8" y2="30" width="1.6" layer="92"/>
<text x="-8" y="14" size="1.1" layer="94">Y1</text>
<text x="1.2" y="-28" size="1.1" layer="94">Y2</text>
<text x="9.2" y="24" size="1.1" layer="94">U3</text>
</symbol>
<symbol name="TIRE">
<text x="0" y="12" size="1.6" layer="95" align="bottom-center">&gt;NAME</text>
<circle x="0" y="0" radius="8" width="0.45" layer="94"/>
<circle x="0" y="0" radius="4" width="0.25" layer="94"/>
<wire x1="-8" y1="0" x2="-14" y2="0" width="1.6" layer="92"/>
</symbol>
<symbol name="EXHAUST">
<text x="0" y="6" size="1.5" layer="95" align="bottom-center">&gt;NAME</text>
<wire x1="0" y1="8" x2="0" y2="0" width="1.6" layer="92"/>
<wire x1="-4" y1="3" x2="0" y2="-2" width="0.3" layer="94"/>
<wire x1="4" y1="3" x2="0" y2="-2" width="0.3" layer="94"/>
<wire x1="-5" y1="-4" x2="5" y2="-4" width="0.25" layer="94"/>
<wire x1="-3" y1="-6.5" x2="3" y2="-6.5" width="0.25" layer="94"/>
<wire x1="-1.5" y1="-9" x2="1.5" y2="-9" width="0.25" layer="94"/>
</symbol>

</symbols>
<devicesets>
<deviceset name="VCC" prefix="P" uservalue="yes">
<gates><gate name="G$1" symbol="VCC" x="0" y="0"/></gates>
<devices><device name=""><technologies><technology name=""/></technologies></device></devices>
</deviceset>
<deviceset name="GND" prefix="GND" uservalue="yes">
<gates><gate name="G$1" symbol="GND" x="0" y="0"/></gates>
<devices><device name=""><technologies><technology name=""/></technologies></device></devices>
</deviceset>
<deviceset name="FUSE" prefix="F" uservalue="yes">
<gates><gate name="G$1" symbol="FUSE" x="0" y="0"/></gates>
<devices><device name=""><technologies><technology name=""/></technologies></device></devices>
</deviceset>
<deviceset name="D" prefix="D" uservalue="yes">
<gates><gate name="G$1" symbol="D" x="0" y="0"/></gates>
<devices><device name=""><technologies><technology name=""/></technologies></device></devices>
</deviceset>
<deviceset name="R" prefix="R" uservalue="yes">
<gates><gate name="G$1" symbol="R" x="0" y="0"/></gates>
<devices><device name=""><technologies><technology name=""/></technologies></device></devices>
</deviceset>
<deviceset name="C" prefix="C" uservalue="yes">
<gates><gate name="G$1" symbol="C" x="0" y="0"/></gates>
<devices><device name=""><technologies><technology name=""/></technologies></device></devices>
</deviceset>
<deviceset name="VALVE" prefix="Y" uservalue="yes">
<gates><gate name="G$1" symbol="VALVE" x="0" y="0"/></gates>
<devices><device name=""><technologies><technology name=""/></technologies></device></devices>
</deviceset>
<deviceset name="SENSOR" prefix="U" uservalue="yes">
<gates><gate name="G$1" symbol="SENSOR" x="0" y="0"/></gates>
<devices><device name=""><technologies><technology name=""/></technologies></device></devices>
</deviceset>
<deviceset name="BOARD" prefix="U" uservalue="yes">
<gates><gate name="G$1" symbol="BOARD" x="0" y="0"/></gates>
<devices><device name=""><technologies><technology name=""/></technologies></device></devices>
</deviceset>


<deviceset name="TANK" prefix="TK" uservalue="yes">
<gates><gate name="G$1" symbol="TANK" x="0" y="0"/></gates>
<devices><device name=""><technologies><technology name=""/></technologies></device></devices>
</deviceset>
<deviceset name="AIRVALVE" prefix="AV" uservalue="yes">
<gates><gate name="G$1" symbol="AIRVALVE" x="0" y="0"/></gates>
<devices><device name=""><technologies><technology name=""/></technologies></device></devices>
</deviceset>
<deviceset name="MANIFOLD" prefix="M" uservalue="yes">
<gates><gate name="G$1" symbol="MANIFOLD" x="0" y="0"/></gates>
<devices><device name=""><technologies><technology name=""/></technologies></device></devices>
</deviceset>
<deviceset name="TIRE" prefix="T" uservalue="yes">
<gates><gate name="G$1" symbol="TIRE" x="0" y="0"/></gates>
<devices><device name=""><technologies><technology name=""/></technologies></device></devices>
</deviceset>
<deviceset name="EXHAUST" prefix="EX" uservalue="yes">
<gates><gate name="G$1" symbol="EXHAUST" x="0" y="0"/></gates>
<devices><device name=""><technologies><technology name=""/></technologies></device></devices>
</deviceset>

</devicesets>
</library>
</libraries>
<parts>
<part name="P12" library="airctl" deviceset="VCC" device="" value="+12V"/>
<part name="F1" library="airctl" deviceset="FUSE" device="" value="2A"/>
<part name="U1" library="airctl" deviceset="BOARD" device="" value="ESP32-Relay-X2"/>
<part name="D1" library="airctl" deviceset="D" device="" value="1N4007"/>
<part name="Y1" library="airctl" deviceset="VALVE" device="" value="INFLATE 12V 6.5W"/>
<part name="D2" library="airctl" deviceset="D" device="" value="1N4007"/>
<part name="Y2" library="airctl" deviceset="VALVE" device="" value="DEFLATE 12V 6.5W"/>
<part name="U3" library="airctl" deviceset="SENSOR" device="" value="0.5-4.5V"/>
<part name="R5" library="airctl" deviceset="R" device="" value="10k"/>
<part name="R6" library="airctl" deviceset="R" device="" value="10k"/>
<part name="C3" library="airctl" deviceset="C" device="" value="100n"/>
<part name="GND1" library="airctl" deviceset="GND" device="" value="GND"/>
<part name="GND2" library="airctl" deviceset="GND" device="" value="GND"/>
<part name="GND3" library="airctl" deviceset="GND" device="" value="GND"/>
<part name="GND4" library="airctl" deviceset="GND" device="" value="GND"/>
<part name="GND5" library="airctl" deviceset="GND" device="" value="GND"/>
<part name="GND6" library="airctl" deviceset="GND" device="" value="GND"/>


<part name="TK1" library="airctl" deviceset="TANK" device="" value="Air tank"/>
<part name="M1" library="airctl" deviceset="MANIFOLD" device="" value="8mm pushfit"/>
<part name="EX1" library="airctl" deviceset="EXHAUST" device="" value="to atmosphere"/>
<part name="T1" library="airctl" deviceset="TIRE" device="" value="Tire"/>
<part name="T2" library="airctl" deviceset="TIRE" device="" value="Tire"/>
<part name="T3" library="airctl" deviceset="TIRE" device="" value="Tire"/>
<part name="T4" library="airctl" deviceset="TIRE" device="" value="Tire"/>

</parts>
<sheets>
<sheet>
<plain>
<text x="15" y="175" size="3.2" layer="97">AirUpDown controller</text>
<text x="15" y="168" size="1.6" layer="97">ESP32 Relay X2. 12V on VCC only. Relay 1 GPIO16 inflate, relay 2 GPIO17 deflate, both active high.</text>
<text x="15" y="163" size="1.6" layer="97">Each valve is 12V 6.5W across NO and GND. 1N4007 across the coil, band toward NO. NC unused.</text>
<text x="15" y="20" size="1.6" layer="97">U3 red +5V, black GND, green signal 0.5-4.5V. 10k/10k divider into GPIO34. Full scale is the PSI stamped on the sensor.</text>
<text x="12" y="157" size="1.8" layer="97">12V supply</text>
<text x="33" y="156" size="1.8" layer="97">F1 Fuse 2A</text>
<text x="74" y="151" size="2" layer="97">U1 ESP32 Relay X2</text>
<text x="168" y="124" size="1.6" layer="97">D1 Flyback</text>
<text x="200" y="60" size="1.6" layer="97">D2 Flyback</text>
<text x="228" y="158" size="1.6" layer="97">U3 Pressure</text>
<text x="242" y="156" size="1.3" layer="97">red</text>
<text x="242" y="146" size="1.3" layer="97">black</text>
<text x="280" y="153" size="1.3" layer="97">green</text>
<text x="62" y="76" size="1.8" layer="97">R5 10k</text>
<text x="88" y="62" size="1.8" layer="97">R6 10k</text>
<text x="94" y="66" size="1.8" layer="97">C3 100n</text>
<text x="53" y="131" size="1.6" layer="97">GND</text>
<text x="246" y="134" size="1.4" layer="97">GND</text>
<text x="88" y="42" size="1.6" layer="97">GND</text>
<text x="103" y="42" size="1.6" layer="97">GND</text>
<text x="186" y="102" size="1.5" layer="97">GND</text>
<text x="220" y="34" size="1.5" layer="97">GND</text>
<wire x1="224" y1="146" x2="224" y2="130" width="2.2" layer="92"/>
<wire x1="238" y1="116" x2="242" y2="116" width="2.2" layer="92"/>
<wire x1="260" y1="70" x2="260" y2="66" width="2.2" layer="92"/>
<wire x1="274" y1="52" x2="292" y2="52" width="2.2" layer="92"/>
<wire x1="268" y1="134" x2="268" y2="130" width="2.2" layer="92"/>
<wire x1="286" y1="115" x2="296" y2="115" width="2.2" layer="92"/>
<wire x1="286" y1="105" x2="296" y2="105" width="2.2" layer="92"/>
<wire x1="286" y1="95" x2="296" y2="95" width="2.2" layer="92"/>
<wire x1="286" y1="85" x2="296" y2="85" width="2.2" layer="92"/>
<text x="196" y="160" size="1.6" layer="97">Air tank</text>
<text x="230" y="142" size="1.3" layer="97">inflate source</text>
<text x="186" y="128" size="1.5" layer="97">Y1 Inflate</text>
<text x="198" y="74" size="1.5" layer="97">Y2 Deflate</text>
<text x="294" y="46" size="1.4" layer="97">Exhaust</text>
<text x="232" y="112" size="1.5" layer="97">8mm manifold</text>
<text x="15" y="8" size="1.45" layer="97">Y1, Y2 and the pressure sensor sit on the manifold. The tank feeds Y1. Y1 fills the manifold and the tires. Y2 vents the manifold. U3 reads that same air.</text>
</plain>
<instances>
<instance part="P12" gate="G$1" x="20" y="150"/>
<instance part="F1" gate="G$1" x="40" y="150"/>
<instance part="U1" gate="G$1" x="95" y="125"/>
<instance part="D1" gate="G$1" x="188" y="113.46" rot="R90"/>
<instance part="Y1" gate="G$1" x="210" y="116"/>
<instance part="D2" gate="G$1" x="224" y="49.46" rot="R90"/>
<instance part="Y2" gate="G$1" x="246" y="52"/>
<instance part="U3" gate="G$1" x="268" y="150"/>
<instance part="R5" gate="G$1" x="70" y="70"/>
<instance part="R6" gate="G$1" x="85" y="55" rot="R90"/>
<instance part="C3" gate="G$1" x="100" y="55"/>
<instance part="GND1" gate="G$1" x="50" y="130.08"/>
<instance part="GND2" gate="G$1" x="257.84" y="136"/>
<instance part="GND3" gate="G$1" x="85" y="40"/>
<instance part="GND4" gate="G$1" x="100" y="40"/>
<instance part="GND5" gate="G$1" x="202.38" y="100"/>
<instance part="GND6" gate="G$1" x="238.38" y="36"/>


<instance part="TK1" gate="G$1" x="208" y="146"/>
<instance part="M1" gate="G$1" x="260" y="100"/>
<instance part="EX1" gate="G$1" x="292" y="44"/>
<instance part="T1" gate="G$1" x="310" y="115"/>
<instance part="T2" gate="G$1" x="310" y="105"/>
<instance part="T3" gate="G$1" x="310" y="95"/>
<instance part="T4" gate="G$1" x="310" y="85"/>

</instances>
<nets>
<net name="+12V" class="0">
<segment>
<wire x1="20" y1="150" x2="34.92" y2="150" width="0.1524" layer="1"/>
<pinref part="P12" gate="G$1" pin="VCC"/>
<pinref part="F1" gate="G$1" pin="1"/>
<label x="22" y="151.5" size="1.27" layer="95"/>
</segment>
</net>
<net name="+12V_FUSED" class="0">
<segment>
<wire x1="45.08" y1="150" x2="45.08" y2="158" width="0.1524" layer="1"/>
<wire x1="45.08" y1="158" x2="136" y2="158" width="0.1524" layer="1"/>
<wire x1="67.06" y1="140.24" x2="67.06" y2="158" width="0.1524" layer="1"/>
<wire x1="122.94" y1="142.78" x2="130" y2="142.78" width="0.1524" layer="1"/>
<wire x1="130" y1="142.78" x2="130" y2="158" width="0.1524" layer="1"/>
<wire x1="122.94" y1="117.38" x2="136" y2="117.38" width="0.1524" layer="1"/>
<wire x1="136" y1="117.38" x2="136" y2="158" width="0.1524" layer="1"/>
<pinref part="F1" gate="G$1" pin="2"/>
<pinref part="U1" gate="G$1" pin="VCC"/>
<pinref part="U1" gate="G$1" pin="COM1"/>
<pinref part="U1" gate="G$1" pin="COM2"/>
<label x="70" y="159.2" size="1.27" layer="95"/>
</segment>
</net>
<net name="GND" class="0">
<segment>
<wire x1="67.06" y1="130.08" x2="50" y2="130.08" width="0.1524" layer="102"/>
<pinref part="U1" gate="G$1" pin="GND"/>
<pinref part="GND1" gate="G$1" pin="GND"/>
</segment>
<segment>
<wire x1="188" y1="108.38" x2="202.38" y2="108.38" width="0.1524" layer="102"/>
<wire x1="202.38" y1="108.38" x2="202.38" y2="113.46" width="0.1524" layer="102"/>
<wire x1="202.38" y1="108.38" x2="202.38" y2="100" width="0.1524" layer="102"/>
<pinref part="Y1" gate="G$1" pin="-"/>
<pinref part="GND5" gate="G$1" pin="GND"/>
<pinref part="D1" gate="G$1" pin="A"/>
</segment>
<segment>
<wire x1="224" y1="44.38" x2="238.38" y2="44.38" width="0.1524" layer="102"/>
<wire x1="238.38" y1="44.38" x2="238.38" y2="49.46" width="0.1524" layer="102"/>
<wire x1="238.38" y1="44.38" x2="238.38" y2="36" width="0.1524" layer="102"/>
<pinref part="Y2" gate="G$1" pin="-"/>
<pinref part="GND6" gate="G$1" pin="GND"/>
<pinref part="D2" gate="G$1" pin="A"/>
</segment>
<segment>
<wire x1="257.84" y1="144.92" x2="257.84" y2="136" width="0.1524" layer="102"/>
<pinref part="U3" gate="G$1" pin="GND"/>
<pinref part="GND2" gate="G$1" pin="GND"/>
</segment>
<segment>
<wire x1="85" y1="49.92" x2="85" y2="40" width="0.1524" layer="102"/>
<pinref part="R6" gate="G$1" pin="1"/>
<pinref part="GND3" gate="G$1" pin="GND"/>
</segment>
<segment>
<wire x1="100" y1="49.92" x2="100" y2="40" width="0.1524" layer="102"/>
<pinref part="C3" gate="G$1" pin="2"/>
<pinref part="GND4" gate="G$1" pin="GND"/>
</segment>
</net>
<net name="+5V" class="0">
<segment>
<wire x1="67.06" y1="119.92" x2="8" y2="119.92" width="0.1524" layer="91"/>
<wire x1="8" y1="119.92" x2="8" y2="192" width="0.1524" layer="91"/>
<wire x1="8" y1="192" x2="257.84" y2="192" width="0.1524" layer="91"/>
<wire x1="257.84" y1="192" x2="257.84" y2="155.08" width="0.1524" layer="91"/>
<pinref part="U1" gate="G$1" pin="5V"/>
<pinref part="U3" gate="G$1" pin="VCC"/>
<label x="22" y="121" size="1.27" layer="95"/>
</segment>
</net>
<net name="VALVE1" class="0">
<segment>
<wire x1="122.94" y1="135.16" x2="170" y2="135.16" width="0.1524" layer="1"/>
<wire x1="170" y1="135.16" x2="170" y2="118.54" width="0.1524" layer="1"/>
<wire x1="170" y1="118.54" x2="202.38" y2="118.54" width="0.1524" layer="1"/>
<pinref part="U1" gate="G$1" pin="NO1"/>
<pinref part="D1" gate="G$1" pin="K"/>
<pinref part="Y1" gate="G$1" pin="+"/>
<label x="158" y="136.4" size="1.27" layer="95"/>
</segment>
</net>
<net name="VALVE2" class="0">
<segment>
<wire x1="122.94" y1="109.76" x2="140" y2="109.76" width="0.1524" layer="1"/>
<wire x1="140" y1="109.76" x2="140" y2="54.54" width="0.1524" layer="1"/>
<wire x1="140" y1="54.54" x2="238.38" y2="54.54" width="0.1524" layer="1"/>
<pinref part="U1" gate="G$1" pin="NO2"/>
<pinref part="D2" gate="G$1" pin="K"/>
<pinref part="Y2" gate="G$1" pin="+"/>
<label x="158" y="111" size="1.27" layer="95"/>
</segment>
</net>
<net name="SENSOR" class="0">
<segment>
<wire x1="278.16" y1="150" x2="278.16" y2="198" width="0.1524" layer="91"/>
<wire x1="278.16" y1="198" x2="4" y2="198" width="0.1524" layer="91"/>
<wire x1="4" y1="198" x2="4" y2="70" width="0.1524" layer="91"/>
<wire x1="4" y1="70" x2="64.92" y2="70" width="0.1524" layer="91"/>
<pinref part="U3" gate="G$1" pin="OUT"/>
<pinref part="R5" gate="G$1" pin="1"/>
<label x="52" y="71.4" size="1.27" layer="95"/>
</segment>
</net>
<net name="ADC" class="0">
<segment>
<wire x1="75.08" y1="70" x2="105.16" y2="70" width="0.1524" layer="91"/>
<wire x1="85" y1="60.08" x2="85" y2="70" width="0.1524" layer="91"/>
<pinref part="R6" gate="G$1" pin="2"/>
<wire x1="100" y1="60.08" x2="100" y2="70" width="0.1524" layer="91"/>
<wire x1="105.16" y1="70" x2="105.16" y2="91.98" width="0.1524" layer="91"/>
<pinref part="R5" gate="G$1" pin="2"/>
<pinref part="C3" gate="G$1" pin="1"/>
<pinref part="U1" gate="G$1" pin="IO34"/>
<label x="90" y="71.4" size="1.27" layer="95"/>
</segment>
</net>
</nets>
</sheet>
</sheets>
</schematic>
</drawing>
</eagle>
