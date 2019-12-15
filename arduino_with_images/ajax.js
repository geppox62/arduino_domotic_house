		strL1 = "";
		strL2 = "";
		strL3 = "";
		strL4 = "";
		strL5 = "";
		strL6 = "";
		strS1 = "";
		strS2 = "";
		strS3 = "";
		strS4 = "";
		strS5 = "";
		strS6 = "";
		valTerm = 23;
		var Stato_intrusione = 0;
		var Stato_riscaldamento = 0;
		var Stato_condizionamento = 0;
		var Stato_serranda = 0;
		var Stato_buzzer = 0;

		function GetArduinoIO()
		{
			nocache = "&nocache=" + Math.random() * 1000000;
			var request = new XMLHttpRequest();
			request.onreadystatechange = function()
			{
				if (this.readyState == 4) {
					if (this.status == 200) {
						if (this.responseXML != null) {
							// Ho ricevuto il file XML con i dati da inserire in pagina
							// Luce camera
							if (this.responseXML.getElementsByTagName('camera')[0].childNodes[0].nodeValue === "on") {
								document.LED_form.camera.checked = true;
							}
							else {
								document.LED_form.camera.checked = false;
							}
							// Luce cucina
							if (this.responseXML.getElementsByTagName('cucina')[0].childNodes[0].nodeValue === "on") {
								document.LED_form.cucina.checked = true;
							}
							else {
								document.LED_form.cucina.checked = false;
							}
							// Luce soggiorno
							if (this.responseXML.getElementsByTagName('soggiorno')[0].childNodes[0].nodeValue === "on") {
								document.LED_form.soggiorno.checked = true;
							}
							else {
								document.LED_form.soggiorno.checked = false;
							}
							// Luce bagno
							if (this.responseXML.getElementsByTagName('bagno')[0].childNodes[0].nodeValue === "on") {
								document.LED_form.bagno.checked = true;
							}
							else {
								document.LED_form.bagno.checked = false;
							}
							// Luce disimpegno
							if (this.responseXML.getElementsByTagName('disimpegno')[0].childNodes[0].nodeValue === "on") {
								document.LED_form.disimpegno.checked = true;
							}
							else {
								document.LED_form.disimpegno.checked = false;
							}
							// Luce garage
							if (this.responseXML.getElementsByTagName('garage')[0].childNodes[0].nodeValue === "on") {
								document.LED_form.garage.checked = true;
							}
							else {
								document.LED_form.garage.checked = false;
							}
							// Temperatura
							document.getElementsByClassName("temperatura")[0].innerHTML =
									this.responseXML.getElementsByTagName('temperatura')[0].childNodes[0].nodeValue;

							// Luci crepuscolari
							if (this.responseXML.getElementsByTagName('crepuscolari')[0].childNodes[0].nodeValue === "on") {
								document.getElementsByClassName("crepuscolari")[0].innerHTML = "LUCI CREPUSCOLARI ACCESE"
							}
							else {
								document.getElementsByClassName("crepuscolari")[0].innerHTML = "LUCI CREPUSCOLARI SPENTE"
							}
							// Intrusione
							if (this.responseXML.getElementsByTagName('intrusione')[0].childNodes[0].nodeValue === "on") {
								document.getElementById("intrusione").innerHTML = "ALLARME ON";
								Stato_intrusione = 1;
							}
							else {
								document.getElementById("intrusione").innerHTML = "ALLARME OFF";
								Stato_intrusione = 0;
							}
							// Rilevato movimento
							if (this.responseXML.getElementsByTagName('movimento')[0].childNodes[0].nodeValue === "on") {
								document.getElementsByClassName("movimento")[0].innerHTML = "INTRUSIONE!!!"
							}
							else {
								document.getElementsByClassName("movimento")[0].innerHTML = "TUTTO OK"
							}
							// Riscaldamento
							if (this.responseXML.getElementsByTagName('riscaldamento')[0].childNodes[0].nodeValue === "on") {
								document.getElementById("riscaldamento").innerHTML = "RISCALDAMENTO ON";
								Stato_riscaldamento = 1;
							}
							else {
								document.getElementById("riscaldamento").innerHTML = "RISCALDAMENTO OFF";
								Stato_riscaldamento = 0;
							}
							// Condizionamento
							if (this.responseXML.getElementsByTagName('condizionamento')[0].childNodes[0].nodeValue === "on") {
								document.getElementById("condizionamento").innerHTML = "CONDIZIONAMENTO ON";
								Stato_condizionamento = 1;
							}
							else {
								document.getElementById("condizionamento").innerHTML = "CONDIZIONAMENTO OFF";
								Stato_condizionamento = 0;
							}
							// Serranda garage
							if (this.responseXML.getElementsByTagName('serranda')[0].childNodes[0].nodeValue === "on") {
								document.getElementById("serranda").innerHTML = "SERRANDA GARAGE APERTA";
								Stato_serranda = 1;
							}
							else {
								document.getElementById("serranda").innerHTML = "SERRANDA GARAGE CHIUSA";
								Stato_serranda = 0;
							}
							// Cicalino
							if (this.responseXML.getElementsByTagName('buzzer')[0].childNodes[0].nodeValue === "on") {
								document.getElementById("buzzer").innerHTML = "SPEGNI CICALINO";
								Stato_buzzer = 1;
							}
							else {
								document.getElementById("buzzer").innerHTML = "ACCENDI CICALINO";
								Stato_buzzer = 0;
							}
							// Allarme gas
							if (this.responseXML.getElementsByTagName('gas')[0].childNodes[0].nodeValue === "on") {
								document.getElementsByClassName("gas")[0].innerHTML = "ALLARME GAS!!!"
							}
							else {
								document.getElementsByClassName("gas")[0].innerHTML = "SENSORE GAS OK"
							}
						}
					}
				}
			}
			// send HTTP GET request with LEDs to switch on/off if any
			request.open("GET", "ajax_inputs" + strL1 + strL2 + strL3 + strL4 + strL5 + strL6 + strS1 + strS2 + strS3 + strS4 + strS5 + strS6 + nocache, true);
			request.send(null);
			setTimeout('GetArduinoIO()', 1000);
			strL1 = "";
			strL2 = "";
			strL3 = "";
			strL4 = "";
			strL5 = "";
			strL6 = "";
			strS1 = "";
			strS2 = "";
			strS3 = "";
			strS4 = "";
			strS5 = "";
			strS6 = "";
		}
		// Controllo le luci quando le checkbox sono checked/unchecked
		function GetCheck()
		{
			// Led camera
			if (LED_form.camera.checked)
				strL1 = "&Cam=1";
			else
				strL1 = "&Cam=0";
			// Led cucina
			if (LED_form.cucina.checked)
				strL2 = "&Cuc=1";
			else
				strL2 = "&Cuc=0";
			// Led soggiorno
			if (LED_form.soggiorno.checked)
				strL3 = "&Sog=1";
			else
				strL3 = "&Sog=0";
			// Led bagno
			if (LED_form.bagno.checked)
				strL4 = "&Bag=1";
			else
				strL4 = "&Bag=0";
			// Led disimpegno
			if (LED_form.disimpegno.checked)
				strL5 = "&Dis=1";
			else
				strL5 = "&Dis=0";
			// Led garage
			if (LED_form.garage.checked)
				strL6 = "&Gar=1";
			else
				strL6 = "&Gar=0";
		}
		function GetIntrusione()
		{
			if (Stato_intrusione === 1) {
				Stato_intrusione = 0;
				strS1 = "&Intr=0";
			}
			else {
				Stato_intrusione = 1;
				strS1 = "&Intr=1";
			}
		}
		function GetRiscaldamento()
		{
			if (Stato_riscaldamento === 1) {
				Stato_riscaldamento = 0;
				strS2 = "&Risc=0";
			}
			else {
				Stato_riscaldamento = 1;
				strS2 = "&Risc=1";
				Stato_condizionamento = 0;
				strS3 = "&Cond=0";
			}
		}
		function GetCondizionamento()
		{
			if (Stato_condizionamento === 1) {
				Stato_condizionamento = 0;
				strS3 = "&Cond=0";
			}
			else {
				Stato_condizionamento = 1;
				strS3 = "&Cond=1";
				Stato_riscaldamento = 0;
				strS2 = "&Risc=0";
			}
		}
		function GetSerranda()
		{
			if (Stato_serranda === 1) {
				Stato_serranda = 0;
				strS4 = "&Serr=0";
			}
			else {
				Stato_serranda = 1;
				strS4 = "&Serr=1";
			}
		}
		function GetBuzzer()
		{
			if (Stato_buzzer === 1) {
				Stato_buzzer = 0;
				strS6 = "&Buzz=0";
			}
			else {
				Stato_buzzer = 1;
				strS6 = "&Buzz=1";
			}
		}
		function GetTermostato()
		{
			valTerm = document.getElementById("select_val").value;
			strS5="&Term=" + valTerm
		}
		function SpegniTutto()
		{
			strL1="&Cam=0";
			strL2="&Cuc=0";
			strL3="&Sog=0";
			strL4="&Bag=0";
			strL5="&Dis=0";
			strL6="&Gar=0";
		}