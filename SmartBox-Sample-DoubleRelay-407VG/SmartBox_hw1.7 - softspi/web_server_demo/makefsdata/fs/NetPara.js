var isfirst = 1;
var xmlhttp;
function loadXMLDoc(url,cfunc)
{
  if (window.XMLHttpRequest)
  {
     xmlhttp=new XMLHttpRequest();
  }
  else
  {
     xmlhttp=new ActiveXObject("Microsoft.XMLHTTP");
  }
  xmlhttp.onreadystatechange=cfunc;
  xmlhttp.open("GET",url,true);
  xmlhttp.send();
}

function NetParaUpdate(){
	loadXMLDoc("/netpara.cgi?t="+ Math.random(),function()
	{
		if (xmlhttp.readyState==4 && xmlhttp.status==200)
		{
			var NetPara;
			NetPara = xmlhttp.responseText;
			NetPara = NetPara.split(";");//分割函数，遇到；就分割
			document.getElementById("sw").innerHTML=NetPara[0];
			document.getElementById("hw").innerHTML=NetPara[1];
			document.getElementById("devid").innerHTML=NetPara[2];
			document.getElementById("devtype").innerHTML=NetPara[3];
			document.getElementById("devip").innerHTML=NetPara[4];
			document.getElementById("netmask").innerHTML=NetPara[5];
			document.getElementById("gateway").innerHTML=NetPara[6];
			document.getElementById("serverip").innerHTML=NetPara[7];
			document.getElementById("serverport").innerHTML=NetPara[8];
			document.getElementById("dhcp").innerHTML=NetPara[9];
			document.getElementById("ntp").innerHTML=NetPara[10];
		}
	});
} 

function NetParaUpdateSet(){
	loadXMLDoc("/netpara.cgi?t="+ Math.random(),function()
	{
		if (xmlhttp.readyState==4 && xmlhttp.status==200)
		{
			var NetPara;
			NetPara = xmlhttp.responseText;
			NetPara = NetPara.split(";");//分割函数，遇到；就分割
//			document.getElementById("sw").innerHTML=NetPara[0];
//			document.getElementById("hw").innerHTML=NetPara[1];
//			document.getElementById("devid").innerHTML=NetPara[2];
//			document.getElementById("devtype").innerHTML=NetPara[3];
			document.getElementById("setdevip").value=NetPara[4];
			document.getElementById("setnetmask").value=NetPara[5];
			document.getElementById("setgateway").value=NetPara[6];
			document.getElementById("setserverip").value=NetPara[7];
			document.getElementById("setserverport").value=NetPara[8];
			if(NetPara[9] == "ON")
			{
				document.getElementById("dhcp_on").checked="checked";
			}
			else if(NetPara[9] == "OFF")
			{
				document.getElementById("dhcp_off").checked="checked";
			} 
			document.getElementById("setntp").value=NetPara[10];
		}
	});
}

function updateRtcTime(){
   loadXMLDoc("/rtctime.cgi?t="+ Math.random(),function()
  {
    if (xmlhttp.readyState==4 && xmlhttp.status==200)
    {
	  var text;
	  text = xmlhttp.responseText;
	  text = text.split(";");//分割函数，遇到；就分割
	  document.getElementById("rtctime").innerHTML=text;
    }
  });
}

function EnvUpdate(){
	loadXMLDoc("/env.cgi?t="+ Math.random(),function()
	{
		if (xmlhttp.readyState==4 && xmlhttp.status==200)
		{
			var EnvPara;
			EnvPara = xmlhttp.responseText;
			EnvPara = EnvPara.split(";");//分割函数，遇到；就分割
			document.getElementById("temp").innerHTML=EnvPara[0];
			document.getElementById("hd").innerHTML=EnvPara[1];
			document.getElementById("pm2_5").innerHTML=EnvPara[2];
			document.getElementById("pm10").innerHTML=EnvPara[3];
			document.getElementById("atm").innerHTML=EnvPara[4];
			document.getElementById("windspeed").innerHTML=EnvPara[5];
			document.getElementById("winddir").innerHTML=EnvPara[6];
			document.getElementById("noise").innerHTML=EnvPara[7];
			if(isfirst)
			{
				if(EnvPara[8] == '1')
					document.getElementById("SM5386").checked = "checked";
				if(EnvPara[9] == '1')
					document.getElementById("SM5387B").checked = "checked";
				if(EnvPara[10] == '1')
					document.getElementById("SM6333B").checked = "checked";
				if(EnvPara[11] == '1')
					document.getElementById("MTC70M").checked = "checked";
				if(EnvPara[12] == '1')
					document.getElementById("IOTB2_16").checked = "checked";
				if(EnvPara[13] == '1')
					document.getElementById("WS301").checked = "checked";
				if(EnvPara[14] == '1')
					document.getElementById("HCD68XX").checked = "checked";
				if(EnvPara[15] == '1')
					document.getElementById("ManholeCover_angle").checked = "checked";
			}
			isfirst = 0;
			document.getElementById("qj_x").innerHTML=EnvPara[16];
			document.getElementById("qj_y").innerHTML=EnvPara[17];
			document.getElementById("qj_z").innerHTML=EnvPara[18];
			document.getElementById("jg_x").innerHTML=EnvPara[19];
			document.getElementById("jg_y").innerHTML=EnvPara[20];
			document.getElementById("jg_z").innerHTML=EnvPara[21];
			document.getElementById("water_level").innerHTML=EnvPara[22];
		}
	});
}

function TimCtrldate(){
	loadXMLDoc("/timectrl.cgi?t="+ Math.random(),function()
	{
		if (xmlhttp.readyState==4 && xmlhttp.status==200)
		{
			var TimeCtrlData;
			TimeCtrlData = xmlhttp.responseText;
			TimeCtrlData = TimeCtrlData.split(";");//分割函数，遇到；就分割
			document.getElementById("start_time1").value=TimeCtrlData[0];
			document.getElementById("led1_tim1").value=TimeCtrlData[1];
			document.getElementById("led1_pwm1").value=TimeCtrlData[2];
			document.getElementById("led1_tim2").value=TimeCtrlData[3];
			document.getElementById("led1_pwm2").value=TimeCtrlData[4];
			document.getElementById("led1_tim3").value=TimeCtrlData[5];
			document.getElementById("led1_pwm3").value=TimeCtrlData[6];
			document.getElementById("led1_tim4").value=TimeCtrlData[7];
			document.getElementById("led1_pwm4").value=TimeCtrlData[8];
			
			document.getElementById("start_time2").value=TimeCtrlData[9];
			document.getElementById("led2_tim1").value=TimeCtrlData[10];
			document.getElementById("led2_pwm1").value=TimeCtrlData[11];
			document.getElementById("led2_tim2").value=TimeCtrlData[12];
			document.getElementById("led2_pwm2").value=TimeCtrlData[13];
			document.getElementById("led2_tim3").value=TimeCtrlData[14];
			document.getElementById("led2_pwm3").value=TimeCtrlData[15];
			document.getElementById("led2_tim4").value=TimeCtrlData[16];
			document.getElementById("led2_pwm4").value=TimeCtrlData[17];
		}
	});
}

function updateLedSta(){
	loadXMLDoc("/ledsta.cgi?t="+ Math.random(),function()
  	{
    	if (xmlhttp.readyState==4 && xmlhttp.status==200)
    	{
			var ledtextpwm;
	  		text = xmlhttp.responseText;
	  		text = text.split(";");//分割函数，遇到；就分割
			if(isfirst)
			{
				document.getElementById("led1").value=text[0];
				document.getElementById("led2").value=text[1];
			}
			document.getElementById("ele1").innerHTML=text[2];
			document.getElementById("ele2").innerHTML=text[3];
			isfirst = 0;
    	}
  });
}

function initLedSta()
{
	setTimeout(updateLedSta,0);
	setTimeout(TimCtrldate,100);
	setInterval(updateLedSta,5000);
}

function initCfg()
{
	setTimeout(NetParaUpdateSet,0);
	setInterval(updateRtcTime,1000);
}

function initHome()
{
	setTimeout(NetParaUpdate,0);
	setTimeout(EnvUpdate,100);
	setInterval(updateRtcTime,1000);
	setInterval(EnvUpdate,5123);
}
