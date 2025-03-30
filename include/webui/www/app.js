// Basic frontend JavaScript
document.addEventListener('DOMContentLoaded', () => {
    console.log('Frontend loaded');
    
    // Example of interacting with the backend
    fetch('/api/hello')
        .then(response => response.json())
        .then(data => console.log('Backend response:', data))
        .catch(error => console.error('Error:', error));
});
