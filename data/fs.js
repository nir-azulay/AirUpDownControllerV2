// File System Management JavaScript

// Function to refresh the file list
function refreshFileList() {
    fetch('/filelist')
        .then(response => response.text())
        .then(html => {
            document.getElementById('fileList').innerHTML = html;
        })
        .catch(error => console.error('Error refreshing file list:', error));
}

// Function to delete a file
function deletef(filename) {
    if (confirm('Are you sure you want to delete ' + filename + '?')) {
        fetch('/delete?file=' + filename)
            .then(response => response.text())
            .then(result => {
                console.log('Delete result:', result);
                refreshFileList();
            })
            .catch(error => console.error('Error deleting file:', error));
    }
}

// Function to handle file upload
function handleFileUpload() {
    const fileInput = document.getElementById('fileInput');
    const file = fileInput.files[0];
    if (!file) {
        alert('Please select a file to upload');
        return;
    }

    const formData = new FormData();
    formData.append('file', file);

    fetch('/doUpload', {
        method: 'POST',
        body: formData
    })
    .then(response => response.text())
    .then(result => {
        console.log('Upload result:', result);
        refreshFileList();
        fileInput.value = ''; // Clear the file input
    })
    .catch(error => console.error('Error uploading file:', error));
}

// Function to handle firmware update
function handleFirmwareUpdate() {
    const fileInput = document.getElementById('firmwareInput');
    const file = fileInput.files[0];
    if (!file) {
        alert('Please select a firmware file to upload');
        return;
    }

    const formData = new FormData();
    formData.append('file', file);

    // Show progress bar
    document.getElementById('updateProgress').style.display = 'block';
    document.getElementById('progressBar').style.width = '0%';

    fetch('/doUpdate', {
        method: 'POST',
        body: formData
    })
    .then(response => response.text())
    .then(result => {
        console.log('Update result:', result);
        alert('Firmware update completed. Device will restart in 30 seconds.');
        setTimeout(() => {
            window.location.href = '/';
        }, 30000);
    })
    .catch(error => {
        console.error('Error updating firmware:', error);
        alert('Error updating firmware: ' + error);
    })
    .finally(() => {
        fileInput.value = ''; // Clear the file input
    });
}

// Initialize the page
document.addEventListener('DOMContentLoaded', function() {
    // Initial file list refresh
    refreshFileList();

    // Set up file upload button
    document.getElementById('uploadButton').addEventListener('click', handleFileUpload);

    // Set up firmware update button
    document.getElementById('updateButton').addEventListener('click', handleFirmwareUpdate);

    // Set up refresh button
    document.getElementById('refreshButton').addEventListener('click', refreshFileList);
}); 