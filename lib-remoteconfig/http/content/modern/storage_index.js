function formatBytes(value) {
	const size = Number(value);
	if (!Number.isFinite(size)) {
		return '';
	}

	if (size < 1024) {
		return `${size} B`;
	}

	if (size < 1024 * 1024) {
		return `${(size / 1024).toFixed(1)} KiB`;
	}

	return `${(size / (1024 * 1024)).toFixed(1)} MiB`;
}

function storageUrl(name) {
	return `/storage/${encodeURIComponent(name)}`;
}

function renderStorageFiles(files) {
	const tbody = document.getElementById('storageFiles');
	if (!tbody) {
		return;
	}

	tbody.innerHTML = '';

	for (const file of files || []) {
		const tr = document.createElement('tr');

		const nameCell = document.createElement('td');
		const link = document.createElement('a');
		link.href = storageUrl(file.name);
		link.textContent = file.name;
		link.download = file.name;
		nameCell.appendChild(link);

		const sizeCell = document.createElement('td');
		sizeCell.textContent = formatBytes(file.size);

		const dateCell = document.createElement('td');
		dateCell.textContent = file.date || '';

		tr.appendChild(nameCell);
		tr.appendChild(sizeCell);
		tr.appendChild(dateCell);

		tbody.appendChild(tr);
	}
}

async function loadStorage() {
	const response = await fetch('/json/storage/directory');

	if (!response.ok) {
		console.error(`Storage directory request failed: HTTP ${response.status}`);
		return;
	}

	const data = await response.json();

	const label = document.getElementById('storageLabel');
	const capacity = document.getElementById('storageCapacity');
	const free = document.getElementById('storageFree');

	if (label) {
		label.textContent = data.label ?? '';
	}

	if (capacity) {
		capacity.textContent = formatBytes(data.capacity);
	}

	if (free) {
		free.textContent = formatBytes(data.free);
	}

	renderStorageFiles(data.files);
}

async function uploadStorageFile() {
	const input = document.getElementById('storageFile');
	const status = document.getElementById('uploadStatus');
	const button = document.getElementById('uploadButton');

	if (!input || !input.files || input.files.length === 0) {
		if (status) {
			status.textContent = 'Select a file.';
		}
		return;
	}

	const file = input.files[0];

	if (status) {
		status.textContent = `Uploading ${file.name}...`;
	}

	if (button) {
		button.disabled = true;
	}

	try {
		const response = await fetch(storageUrl(file.name), {
			method: 'PUT',
			body: file
		});

		if (!response.ok) {
			if (status) {
				status.textContent = `Upload failed: HTTP ${response.status}`;
			}
			return;
		}

		if (status) {
			status.textContent = `Uploaded ${file.name}`;
		}

		input.value = '';
		await loadStorage();
	} catch (error) {
		console.error(error);

		if (status) {
			status.textContent = 'Upload failed.';
		}
	} finally {
		if (button) {
			button.disabled = false;
		}
	}
}
