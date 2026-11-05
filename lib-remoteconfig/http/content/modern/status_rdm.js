(function injectRdmStyle() {
	if (document.getElementById('rdm-style')) return;

	const style = document.createElement('style');
	style.id = 'rdm-style';
	style.textContent =
		'.rdm-table {width:100%;border-collapse:collapse;font-size:12px;margin-bottom:12px;}' +
		'.rdm-table th,.rdm-table td {border:1px solid #555;padding:4px 6px;text-align:center;}' +
		'.rdm-table th {background:#2a2a2a;color:#ccc;}' +
		'.rdm-table td.uid {text-align:left;font-family:monospace;}';
	document.head.appendChild(style);
})();

window.rdm = {
	_path: '',
	_name: '',

	async init(path, name) {
		this._path = path;
		this._name = name;
		await this.load();
	},

	async load() {
		const container = document.getElementById('modules');
		if (!container) return;

		let card = document.getElementById('status-rdm-card');
		if (!card) {
			card = document.createElement('div');
			card.id = 'status-rdm-card';
			card.className = 'card';
			container.appendChild(card);
		}

		const data = await getJSON(this._path);
		if (!data || !Array.isArray(data.rdm)) {
			card.innerHTML = '<h2>' + escapeHtml(this._name) + '</h2>' +
				'<p>RDM status unavailable.</p>' +
				'<div class="row"><button onclick="rdm.load()">Refresh</button></div>';
			return;
		}

		const ports = data.rdm.filter(p => p && /^[A-D]$/.test(p.port));
		const enabled = ports.filter(p => p.enabled === true);
		const todResults = await Promise.all(enabled.map(async p => ({
			port: p.port,
			data: await getJSON(this._path + '/tod?' + p.port)
		})));

		const td = value => '<td>' + escapeHtml(value) + '</td>';
		const yesNo = value => value === true ? 'Yes' : 'No';
		const type = value => value === 'f' ? 'Full' : value === 'i' ? 'Incremental' : (value ?? '-');

		let html = '<h2>' + escapeHtml(this._name) + '</h2>' +
			'<h3>Discovery status</h3>' +
			'<table class="rdm-table"><thead><tr>' +
			'<th>Port</th><th>Enabled</th><th>Waiting</th><th>Type</th><th>Background</th><th>Running</th>' +
			'</tr></thead><tbody>';

		for (const p of ports) {
			html += '<tr>' + td(p.port) + td(yesNo(p.enabled)) +
				td(yesNo(p.waiting)) + td(type(p.type)) +
				td(yesNo(p.bg)) + td(yesNo(p.running)) + '</tr>';
		}
		html += '</tbody></table>';

		if (enabled.length) {
			html += '<h3>Table of Devices (ToD)</h3>' +
				'<table class="rdm-table"><thead><tr><th>Port</th><th>UID</th></tr></thead><tbody>';
			for (const result of todResults) {
				const valid = result.data && result.data.port === result.port && Array.isArray(result.data.tod);
				if (!valid) {
					html += '<tr>' + td(result.port) + '<td>Unavailable</td></tr>';
					continue;
				}
				if (result.data.tod.length === 0) {
					html += '<tr>' + td(result.port) + '<td>No devices</td></tr>';
					continue;
				}
				for (const uid of result.data.tod) {
					html += '<tr>' + td(result.port) + '<td class="uid">' + escapeHtml(uid) + '</td></tr>';
				}
			}
			html += '</tbody></table>';
		}

		html += '<div class="row"><button onclick="rdm.load()">Refresh</button></div>';
		card.innerHTML = html;
	}
};
