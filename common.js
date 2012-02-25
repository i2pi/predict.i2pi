
var csv_json;
var res_json;
var req, req2;

var svgNS = "http://www.w3.org/2000/svg";

var fill_color = '#222';
var plot_feint_col = '#222';
var background_col = '#444';
var label_col = '#aaa';
var r2_label_col = '#f55';


var fileID;

function abortion (e)
{
	while (e.hasChildNodes()) e.removeChild (e.firstChild);
}

function setChildText (id, str)
{
	var s = document.getElementById(id);
	var t = document.createTextNode(str);
	abortion(s);
	s.appendChild(t);
}

function setValueText (id, str, jref)
{
	var s = document.getElementById(id);
	s.value = str;
	s.json_ref = jref;
	s.onchange = function() {edit(this)};
}



function createOption (str)
{
	var o = document.createElement('option');
	var t = document.createTextNode (str);
	o.appendChild(t);

	return (o);
}

function typeDropdown (jref, dt)
{
	var s = document.createElement('select');
	var types = ["Binary", "Integer", "Numeric", "Date", "Factor", "Text"];
	var si = 0;

	for (var t in types)
	{
		s.appendChild(createOption(types[t]));
		if (types[t] == dt) si = t;
	}

	s.json_ref = jref;
	s.onchange = function() {edit(this)};
	s.selectedIndex = si;

	return (s);
}

function prettySize (sz)
{
	try {		
		sz = parseFloat(sz);
		i = 1000;
		if (sz < i) return sprintf ("%.2f", sz);
		i = i * 1000;
		if (sz < i) return sprintf ("%.2fk", sz*1000 / i);
		i = i * 1000;
		if (sz < i) return sprintf ("%.2fm", sz*1000 / i);
		i = i * 1000; 
		if (sz < i) return sprintf ("%.2fb", sz*1000 / i);
		return (sz);
	} catch (err)
	{
		return (sz);
	}
}

function prettyFileSize (sz)
{
	try {		
		sz = parseFloat(sz);
		i = 1000;
		if (sz < i) return sprintf ("%.2f", sz);
		i = i * 1000;
		if (sz < i) return sprintf ("%.2f k", sz*1000 / i);
		i = i * 1000;
		if (sz < i) return sprintf ("%.2f M", sz*1000 / i);
		i = i * 1000; 
		return sprintf ("%.2f G", sz*1000 / i);
	} catch (err)
	{
		return (sz);
	}
}


function sendMeta ()
{
	req = new XMLHttpRequest();
	if(req) {
		var params = 'fileID=' + fileID + '&csv=' + JSON.stringify(csv_json);
		req.open("POST", "edit.cgi", true);
		req.onreadystatechange = processReqChange;
		req.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
		req.setRequestHeader("Content-length", params.length);
		req.setRequestHeader("Connection", "close");
		req.send(params);
	}
}

function makeEdit (jref, text)
{
	eval(jref + " = \"" + text + "\";");
}
	
function sendEdit (jref, text)
{
	makeEdit (jref, text);
	sendMeta();
}


function edit(s)
{
	text = s.value;
	sendEdit (s.json_ref, text);
}

function editColName(s)
{
	text = s.value;
	if (text.substring(0,1) == '*')
	{
		p = 1;
	} else
	{
		p = 0;
	}
	makeEdit (s.json_ref_predict, p);
	sendEdit (s.json_ref, text);
}



function editPredict (i)
{
	sendEdit (i.json_ref, i.checked ? 1 : 0);	
	if (i.checked)
	{	
		i.offsetParent.className = 'predict-column';
	} else
	{
		i.offsetParent.className = 'column';
	}
}

function showClaim ()
{
	setValueText('username', "username", "csv_json.username");
	csv_json.password = null;
	setValueText('password', "password", "csv_json.password");

	var d = document.getElementById ('claim');
	d.style.display="block";

	d = document.getElementById ('claim-link');
	d.style.display="none";
}


function claim ()
{
	var p = document.getElementById('password');
	var u = document.getElementById('username');

	if (p.value.length < 5)
	{
		alert("Password too short.");
	} else
	if (p.value == "password")
	{
		alert ("Pick a better password.");
	} else
	if (u.value == "username")
	{
		alert("Um. That\'s _my_ username");
	} else
	{
		eval(p.json_ref + " = \"" + p.value + "\";");
		eval(u.json_ref + " = \"" + u.value + "\";");
		sendMeta(processReqChange);
	}
}

function svg_line (x, y, x1, y1, col)
{
	var l = document.createElementNS(svgNS, 'line');
	l.setAttributeNS(null, 'x1', x);
	l.setAttributeNS(null, 'y1', y);
	l.setAttributeNS(null, 'x2', x1);
	l.setAttributeNS(null, 'y2', y1);
	l.setAttributeNS(null, 'stroke', col);

	return(l);
}

function svg_circle (x, y, r, col, fill)
{
	var l = document.createElementNS(svgNS, 'circle');
	l.setAttributeNS(null, 'cx', x);
	l.setAttributeNS(null, 'cy', y);
	l.setAttributeNS(null, 'r', r);
	l.setAttributeNS(null, 'stroke', col);
	l.setAttributeNS(null, 'fill', fill);

	return(l);
}

function svg_text (x,y, col, text)
{
	l = document.createElementNS(svgNS, 'text');
	l.setAttributeNS(null, "x", x);
	l.setAttributeNS(null, "y", y);
	l.setAttributeNS(null, "text-anchor", 'start');
	l.setAttributeNS(null, "fill", background_col);
	l.setAttributeNS(null, "stroke", col);
	l.setAttributeNS(null, "font-family", 'Courier, monospace');
	l.setAttributeNS(null, "font-size", '8pt');
	l.setAttributeNS(null, "font-weight", 'normal');
	var t = document.createTextNode(text);
    l.appendChild(t);

	return (l)
}

Array.max = function( array )
{
    return Math.max.apply( Math, array );
};

Array.min = function( array )
{
    return Math.min.apply( Math, array );
};

function trim (x, min, max)
{
	return (x < min) ? min : ( (x > max) ? max : x );
}

function boxplot(rs, s, bx,by,w,h)
{
	x1 = bx;
	x2 = bx+w;
	xm = bx+w/2;

	l  = svg_line (xm,by, xm,by+h, plot_feint_col);
	l.setAttributeNS(null, "stroke-dasharray", "1,2");
	l.setAttributeNS(null, "stroke-width", "1");
	s.appendChild(l);

	l  = svg_line (x1,by, x2,by, plot_feint_col);
	l.setAttributeNS(null, "stroke-dasharray", "1,2");
	l.setAttributeNS(null, "stroke-width", "1");
	s.appendChild(l);
	l  = svg_line (x1,by+h, x2,by+h, plot_feint_col);
	l.setAttributeNS(null, "stroke-dasharray", "1,2");
	l.setAttributeNS(null, "stroke-width", "1");
	s.appendChild(l);


	l = svg_line (xm, by + h - rs.q0 * h, xm, by + h - rs.q100 * h, r2_label_col);
	s.appendChild(l);

	l = svg_line (x1, by + h - rs.q25 * h, x1, by + h - rs.q75 * h, r2_label_col);
	s.appendChild(l);
	l = svg_line (x2, by + h - rs.q25 * h, x2, by + h - rs.q75 * h, r2_label_col);
	s.appendChild(l);
	
	l = svg_line (x1, by + h - rs.q25 * h, x2, by + h - rs.q25 * h, r2_label_col);
	s.appendChild(l);
	l = svg_line (x1, by + h - rs.q75 * h, x2, by + h - rs.q75 * h, r2_label_col);
	s.appendChild(l);
	l = svg_line (x1, by + h - rs.q50 * h, x2, by + h - rs.q50 * h, r2_label_col);
	s.appendChild(l);

	t = svg_text (x1 , by-2, r2_label_col, sprintf ("%.2f", rs.q50));
	s.appendChild(t);

	t = svg_text (x1 , by-13, r2_label_col, "Fit");
	s.appendChild(t);

	return (s);
}

function plot (d, s, bx,by,w,h, title)
{
	l = svg_line (bx,by+h, bx+w,by, plot_feint_col);
	l.setAttributeNS(null, 'stroke-width', 1);
	l.setAttributeNS(null, 'stroke-dasharray', "3,2");
	s.appendChild(l);

	l = svg_line (bx,by, bx,by+h, plot_feint_col);
	s.appendChild(l);
	l = svg_line (bx,by+h, bx+w,by+h, plot_feint_col);
	s.appendChild(l);


	t = svg_text (bx+w/2, by+h/2 + 12, plot_feint_col, title)
	l.setAttributeNS(null, "font-family", 'Helvetica, Arial, sans-serif');
	l.setAttributeNS(null, "text-anchor", 'middle');
	l.setAttributeNS(null, "font-size", '24pt');
	s.appendChild(t);

	amin = Array.min(d.actual);
	amax = Array.max(d.actual);
	pmin = Array.min(d.predict);
	pmax = Array.max(d.predict);
	amin = (amin < pmin) ? amin : pmin;
	amax = (amax > pmax) ? amax : pmax;
	for (var j in d.actual)
	{
		x = bx + w * (d.actual[j] - amin) / (amax - amin);
		y = by + (w - w * (d.predicted[j] - amin) / (amax - amin));
		s.appendChild(svg_circle (x,y, 0.5, '#f70', '#f70'));
	}

	t = svg_text (bx+w - 30, by + h, label_col, "actual");
	s.appendChild(t);

	t = svg_text (bx, by+10, label_col, "predicted");
	s.appendChild(t);

}

function confusion_plot (d, s, bx,by,w,h, title)
{
	t = svg_text (bx+w/2, by+h/2 + 12, plot_feint_col, title)
	l.setAttributeNS(null, "font-family", 'Helvetica, Arial, sans-serif');
	l.setAttributeNS(null, "text-anchor", 'middle');
	l.setAttributeNS(null, "font-size", '24pt');
	s.appendChild(t);

	nLevels = 0; 	// This should be un the results object, but isn't for some reason
	for (var j in d.actual) if(d.actual[j] > nLevels) nLevels = d.actual[j];

	var maxR;
	if (w < h)
	{
		maxR = w / nLevels / 2;
	} else
	{
		maxR = h / nLevels / 2;
	}

	maxR -= 1;

	for (var j=0; j<nLevels-1; j++)
	{
		var hx = bx + w * (j+1) / (nLevels-1);
		var vl = svg_line (hx, by, hx, by+h, plot_feint_col);
		vl.setAttributeNS(null, 'stroke-dasharray', "1,2");
		s.appendChild(vl);
	}

	var vl = svg_line (bx, by, bx+w, by+h, plot_feint_col);
	vl.setAttributeNS(null, 'stroke-dasharray', "3,2");
	s.appendChild(vl);
	

	for (var i=1; i<=nLevels; i++)
	for (var j=1; j<=nLevels; j++)
	{
		var x = bx + w * (i+0.5-1) / (nLevels-1);
		var y = by + h * (j+0.5-1) / (nLevels-1);
		var r = 0;
		var maxCount = 0;
		for (k in d.actual)
		{
			if (d.actual[k] == j) maxCount++;
			if ((d.actual[k] == j) && (d.predicted[k] == i)) r++;
		}
		r  = 1 - Math.log(r / maxCount + 0.01) / Math.log(0.01);
		r *= maxR;


		s.appendChild(svg_circle (x,y,r, '#f70', '#f70'));
	}

	t = svg_text (bx+w - 30, by + h, label_col, "actual");
	s.appendChild(t);

	t = svg_text (bx, by+10, label_col, "predicted");
	s.appendChild(t);
}




function updateResults ()
{
	r = document.getElementById('results')
	abortion (r);
/*
	h = document.createElement('h2');	
	h.appendChild(document.createTextNode('Results'));
	r.appendChild(h);	
*/

	if ((res_json == null) || (res_json.length == 0))
	{
		i = document.createElement('span');
		i.className = 'info';
		t = document.createTextNode("Our elves are working on finding a good model. You can check their status at ");
		a = document.createElement ('a');
		a.href = 'http://twitter.com/i2predict';
		a.appendChild(document.createTextNode("@i2predict"));  
		i.appendChild(t);
		i.appendChild(a);
		i.appendChild (document.createTextNode(" Try back later or ping i.predict.bugs@i2pi.com"));
		r.appendChild(i);

		return;
	}

	for (var i in res_json)
	{
		res = res_json[i];
		
		d = document.createElement('div');
		d.className = 'result';

		s = document.createElement('span');
		s.className = 'info';
		s.appendChild(document.createTextNode('Response: ' + res.responseVariable));

		if (res.unknownData)
		{
			a = document.createElement('a');
			a.className = 'measure';
			a.href="http://predict.i2pi.com/unknowns/" + res.unknownData;
			a.appendChild(document.createTextNode("(Predictions CSV)"));
			s.appendChild(a);
		}


		d.appendChild(s);

		s = document.createElement('span');
		s.className = 'info';
		s.appendChild(document.createTextNode('Measure of Fit: ' + res.measureName));
		f = document.createElement('span')
		f.className ='measure';
		f.appendChild(document.createTextNode(sprintf("%.2f", res.test.measure.q50)));
		s.appendChild(f);
		d.appendChild(s);

		s = document.createElement('span');
		s.className = 'info';
		s.appendChild(document.createTextNode('Response Transform: '));
		// Yes. I am drunk.
		if (!res.responseTransofmName)
		{
			res.responseTransofmName = 'passThrough';
		}
		a = document.createElement('a');
		a.href="http://predict.i2pi.com/transforms/" + res.responseTransofmName + ".R";
		a.appendChild(document.createTextNode(res.responseTransofmName));
		s.appendChild(a);
		d.appendChild(s);

		s = document.createElement('span');
		s.className = 'info';
		s.appendChild(document.createTextNode('Transform: '));
		a = document.createElement('a');
		a.href="http://predict.i2pi.com/transforms/" + res.transformName + ".R";
		a.appendChild(document.createTextNode(res.transformName));
		s.appendChild(a);
		d.appendChild(s);

		s = document.createElement('span');
		s.className = 'info';
		s.appendChild(document.createTextNode('Model: '));
		a = document.createElement('a');
		a.href="http://predict.i2pi.com/models/" + res.modelName + ".R";
		a.appendChild(document.createTextNode(res.modelName));
		s.appendChild(a);
		d.appendChild(s);

		

		var g = document.createElement('div');
		g.className = 'plot';
		var s = document.createElementNS(svgNS, 'svg');
		s.setAttributeNS(null, "viewBox", "0 0 310 100");

		if (res.measureName != 'R Squared')
		{
			confusion_plot(res.train, s, 0,0, 100, 100, 'train');
		} else
		{
			plot(res.train, s, 0,0, 100, 100, 'train');
		}
		boxplot(res.train.measure, s, 110,20, 15, 60);

		if (res.measureName != 'R Squared')
		{
			confusion_plot(res.test, s, 160,0, 100, 100, 'test');
		} else
		{
			plot(res.test, s, 160,0, 100, 100, 'test');
		}
		boxplot(res.test.measure, s, 270,20, 15, 60);

		g.appendChild(s);
		d.appendChild(g);

		r.appendChild(d);
	}	
}

function removeHistText (id, pct, bin)
{
	g = document.getElementById(id);
	abortion(g);
}

function histText (id, pct, bin)
{
	g = document.getElementById(id);
	abortion(g);
	text = bin + " (" + sprintf("%.2f", pct*100) + "%)";
	g.appendChild(svg_text(2,0,label_col,text));
}





function update ()
{
	if (!csv_json.name) csv_json.name = fileID;

	if (!csv_json.description) 
	{
		csv_json.description = "description..."	;
	} 
	setValueText ('description', csv_json.description, "csv_json.description");

	setValueText ('name', csv_json.name, "csv_json.name");
	if (csv_json.owner) 
	{
		setChildText ('owner', csv_json.owner);
	} 

	setChildText ('stats-size', prettyFileSize(csv_json.size) + 'B');
	setChildText ('stats-rows', prettySize(csv_json.estRows));

	var cl = document.getElementById ('columns');
	abortion (cl);
	for (var c in csv_json.column)
	{
		var col=csv_json.column[c];
		var d = document.createElement('div');
		d.className='column';
	
		var i = document.createElement('input');
		i.className = 'editable';
		i.value = col.name;
		if (col.predict == 1)
		{
			i.value = "*" + col.name;
		}
		i.json_ref = "csv_json.column[" + c + "].name";
		i.json_ref_predict = "csv_json.column[" + c + "].predict";
		i.onchange = function() {editColName(this)};
		d.appendChild(i);
		cl.appendChild(d);

		var p = document.createElement('p');
		p.appendChild(document.createTextNode(col.dataType));
		p.className='info';
		d.appendChild(p);

		var s = document.createElementNS(svgNS, 'svg');
		s.id = 'column-' + c;
		s.setAttributeNS(null, "viewBox", "0 0 140 40");


		var h = col.stats.histogram;
		var px =2 ;
		var max_count = 0;
		var total_count = 0;
		var height = 40;


		for (var b in h.bins) 
		{
			if (h.counts[b] > max_count) max_count = h.counts[b];
			total_count += h.counts[b];
		}
		for (var b in h.bins)
		{
			var x = px + 140 / h.bins.length;
			var y = height - (height*h.counts[b] / max_count);

			var l = document.createElementNS(svgNS, 'rect');
			l.setAttributeNS(null, 'x', px);
			l.setAttributeNS(null, 'width', (x-px));
			l.setAttributeNS(null, 'y', 0);
			l.setAttributeNS(null, 'height', height);
			l.setAttributeNS(null, 'stroke', background_col);
			l.setAttributeNS(null, 'fill', background_col);
			l.hist_percent = h.counts[b] / total_count;
			if ((col.dataType == 'Numeric') || (col.dataType == 'Integer'))
			{
				l.hist_bin = prettySize(h.bins[b]);
			} else
			{
				l.hist_bin = h.bins[b];
			}
			l.div_id = 'column-' + c + '-txt';
			l.addEventListener('mouseover', function(e) {histText(e.target.div_id, e.target.hist_percent, e.target.hist_bin);}, false);
			l.addEventListener('mouseout', function(e) {removeHistText(e.target.div_id);}, false);
			s.appendChild(l);


			l = document.createElementNS(svgNS, 'rect');
			l.setAttributeNS(null, 'x', px);
			l.setAttributeNS(null, 'width', (x-px));
			l.setAttributeNS(null, 'y', y);
			l.setAttributeNS(null, 'height', height-y);
			l.setAttributeNS(null, 'stroke', background_col);
			l.setAttributeNS(null, 'stroke-width', 0.2);
			l.setAttributeNS(null, 'fill', plot_feint_col);
			l.hist_percent = h.counts[b] / total_count;
			if ((col.dataType == 'Numeric') || (col.dataType == 'Integer'))
			{
				l.hist_bin = prettySize(h.bins[b]);
			} else
			{
				l.hist_bin = h.bins[b];
			}
			l.div_id = 'column-' + c + '-txt';
			l.addEventListener('mouseover', function(e) {histText(e.target.div_id, e.target.hist_percent, e.target.hist_bin);}, false);
			l.addEventListener('mouseout', function(e) {removeHistText(e.target.div_id);}, false);
			s.appendChild(l);

			px = x;
		}
	
		var g = document.createElementNS(svgNS, 'g');
		g.id = 'column-' + c + '-txt';
		s.appendChild(g);

		l = svg_line(0,height,140,height, plot_feint_col);
		l.setAttributeNS(null, "stroke-dasharray", "1,2");
		s.appendChild(l);


		var p = document.createElement('div');
		p.className = "column-plot";
		p.appendChild(s);

		d.appendChild(p);

		if (col.predict == 1) 
		{
			i.checked = true;
			d.className = 'predict-column';
		}

	}
}

function parseError()
{
	var d = document.getElementById('main');
	abortion(d);
	var e = document.createElement('div');
	e.className = 'error'
	var t = document.createElement('h2');
	t.appendChild(document.createTextNode('We have a problem'));
	e.appendChild(t);

	t = document.createElement('blockquote');
	t.appendChild(document.createTextNode(csv_json.error.message));
	e.appendChild(t);

	t = document.createElement('blockquote');
	t.appendChild(document.createTextNode(csv_json.error.context));
	e.appendChild(t);
	
	d.appendChild(e);
}

function processReqChange() {
    if (req.readyState == 4) {
        if (req.status == 200) {
			eval ("csv_json = " + req.responseText + ";");
			if (csv_json.error)
			{
				parseError();
			} else
			{
				update();
			}
        } else {
            alert("There was a problem retrieving the XML data:\n" +
                req.statusText);
        }
    } 
}

function processReqResChange() {
    if (req2.readyState == 4) {
        if (req2.status == 200) {
			eval ("res_json = " + req2.responseText + ";");
        } else {
			res_json = null;
        }
		updateResults();
    } 
}



function myLoad (url)
{
	req = new XMLHttpRequest();
	fileID = url;
	if(req) {
		req.onreadystatechange = processReqChange;
		url = 'data/' + url + '.json';
		req.open("GET", url, true);
		req.send(null);
	}

	req2 = new XMLHttpRequest();
	if(req2) {
		req2.onreadystatechange = processReqResChange;
		url = 'data/results/' + fileID + '.json';
		req2.open("GET", url, true);
		req2.send(null);
	}
}
