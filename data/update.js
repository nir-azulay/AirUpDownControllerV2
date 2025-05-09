{
    function sub(obj) {
        if (!obj.files || !obj.files[0]) {
            document.getElementById('file-input').textContent = 'לא נבחר קובץ';
            document.getElementById('updateBtn').disabled = true;
            return;
        }

        const fileName = obj.files[0].name;
        document.getElementById('file-input').textContent = 'הקובץ שנבחר: ' + fileName;
        
        if (!fileName.toLowerCase().endsWith('.bin')) {
            document.getElementById('file-input').textContent = 'שגיאה: יש לבחור קובץ מסוג .bin';
            document.getElementById('updateBtn').disabled = true;
            return;
        }

        document.getElementById('updateBtn').disabled = false;
        document.getElementById('progress-container').style.display = 'block';
        document.getElementById('prg').textContent = 'מוכן להעלאה';
    }

    document.addEventListener('DOMContentLoaded', function() {
        const form = document.getElementById('upload_form');
        
        form.addEventListener('submit', function(e) {
            e.preventDefault();
            
            const fileInput = document.getElementById('update');
            if (!fileInput.files || !fileInput.files[0]) {
                document.getElementById('prg').textContent = 'שגיאה: לא נבחר קובץ';
                return;
            }

            document.getElementById('updateBtn').disabled = true;
            document.getElementById('prg').textContent = 'מתחיל בהעלאה...';
            
            const formData = new FormData(form);
            const xhr = new XMLHttpRequest();
            
            xhr.upload.addEventListener('progress', function(evt) {
                if (evt.lengthComputable) {
                    const percentComplete = (evt.loaded / evt.total) * 100;
                    document.getElementById('bar').style.width = percentComplete + '%';
                    document.getElementById('prg').textContent = 'התקדמות: ' + Math.round(percentComplete) + '%';
                }
            });
            
            xhr.addEventListener('load', function() {
                if (xhr.status === 200) {
                    document.getElementById('prg').textContent = 'העדכון הושלם בהצלחה! המערכת תאתחל את עצמה...';
                    document.getElementById('bar').style.width = '100%';
                    setTimeout(function() {
                        window.location.href = '/';
                    }, 3000);
                } else {
                    document.getElementById('prg').textContent = 'שגיאה בעדכון: ' + (xhr.responseText || xhr.statusText);
                    document.getElementById('updateBtn').disabled = false;
                    document.getElementById('bar').style.width = '0%';
                }
            });
            
            xhr.addEventListener('error', function() {
                document.getElementById('prg').textContent = 'שגיאה בתקשורת עם השרת';
                document.getElementById('updateBtn').disabled = false;
                document.getElementById('bar').style.width = '0%';
            });
            
            xhr.addEventListener('abort', function() {
                document.getElementById('prg').textContent = 'העלאה בוטלה';
                document.getElementById('updateBtn').disabled = false;
                document.getElementById('bar').style.width = '0%';
            });
            
            xhr.open('POST', '/doUpdate');
            xhr.send(formData);
        });
    });
}
