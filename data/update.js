{
    function sub(obj) {
        var a = obj.value;
        console.log(a);
        var fileName = a.replace(/^.*[\\\/]/, '')
        console.log(fileName);
        document.getElementById('file-input').innerHTML = fileName;
        document.getElementById('updateBtn').disabled = false;
        document.getElementById('prgbar').style.display = 'block';
        document.getElementById('prg').style.display = 'block';
    };
    function submitForm(e)
    {
        
        document.getElementById('updateBtn').disabled = "disabled";
        e.preventDefault();
        var form = $('#upload_form')[0];
        var data = new FormData(form);

        $.ajax({
            url: '/doUpdate',
            type: 'POST',
            data: data,
            contentType: false,
            processData: false,
            xhr: function () {
                var xhr = new window.XMLHttpRequest();
                xhr.upload.addEventListener('progress', function (evt) {
                    if (evt.lengthComputable) {
                        var per = evt.loaded / evt.total;
                        $('#prg').html('Progress: ' + Math.round(per * 100) + "%");
                        $('#bar').css('width', Math.round(per * 100) + "%");
                    }
                }, false);
                return xhr;
            },
            success: function (d, s) {
                console.log('success!');
                alert("Successfully updated");
                setTimeout("location.href = '../';", 2000);

            },
            error: function (a, b, c) {
            }
        });
    }
}
