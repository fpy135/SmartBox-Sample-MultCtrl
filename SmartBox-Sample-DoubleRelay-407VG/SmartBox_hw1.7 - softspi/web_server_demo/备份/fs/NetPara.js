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
			document.getElementById("devid").innerHTML=NetPara[0];
			document.getElementById("devtype").innerHTML=NetPara[1];
			document.getElementById("devip").innerHTML=NetPara[2];
			document.getElementById("netmask").innerHTML=NetPara[3];
			document.getElementById("gateway").innerHTML=NetPara[4];
			document.getElementById("serverip").innerHTML=NetPara[5];
			document.getElementById("serverport").innerHTML=NetPara[6];
			document.getElementById("dhcp").innerHTML=NetPara[7];
			document.getElementById("ntp").innerHTML=NetPara[8];
//			console.log(NetPara);
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
//			document.getElementById("devid").innerHTML=NetPara[0];
//			document.getElementById("devtype").innerHTML=NetPara[1];
			document.getElementById("setdevip").value=NetPara[2];
			document.getElementById("setnetmask").value=NetPara[3];
			document.getElementById("setgateway").value=NetPara[4];
			document.getElementById("setserverip").value=NetPara[5];
			document.getElementById("setserverport").value=NetPara[6];
			if(NetPara[7] == "ON")
			{
				document.getElementById("dhcp_on").checked="checked";
			}
			else if(NetPara[7] == "OFF")
			{
				document.getElementById("dhcp_off").checked="checked";
			} 
//			document.getElementById("setdhcp").value=NetPara[7];
			document.getElementById("setntp").value=NetPara[8];
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
//			console.log(EnvPara);
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
			console.log(TimeCtrlData);
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
			var ledpwm;
	  		ledpwm = xmlhttp.responseText;
	  		ledpwm = ledpwm.split(";");//分割函数，遇到；就分割
//			console.log(ledpwm);
	  		document.getElementById("led1").value=ledpwm[0];
	  		document.getElementById("led2").value=ledpwm[1];
    }
  });
}

function initLedSta()
{
	setTimeout(updateLedSta,0);
	setTimeout(TimCtrldate,100);
//	setInterval(updateLedSta,1000);
}

function initHome()
{
	setTimeout(NetParaUpdate,0);
	setTimeout(EnvUpdate,100);
	setInterval(updateRtcTime,1000);
	setInterval(EnvUpdate,5010);
}

function initSetSys()
{
	setInterval(updateRtcTime,1000);
}