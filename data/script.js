// Función para formatear la temperatura
const formatTemperature = (temp) => `${temp.toFixed(1)}°C`;

// Función para formatear la cantidad
const formatQuantity = (qty) => `${qty.toFixed(0)} ml`;

// Función para actualizar el estado de enfriamiento
function updateCoolingStatus(needsCooling, timeRemaining) {
    const statusElement = document.getElementById('cooling-status');
    const statusText = statusElement.querySelector('.status-text');
    const coolingTime = document.getElementById('cooling-time');
    
    statusElement.classList.remove('needs-cooling', 'cooling-active', 'optimal-temp');
    
    if (needsCooling) {
        if (timeRemaining > 0) {
            statusElement.classList.add('cooling-active');
            statusText.textContent = 'Enfriando';
            coolingTime.textContent = `Tiempo restante: ${timeRemaining} minutos`;
        } else {
            statusElement.classList.add('needs-cooling');
            statusText.textContent = 'Requiere enfriamiento';
            coolingTime.textContent = '';
        }
    } else {
        statusElement.classList.add('optimal-temp');
        statusText.textContent = 'Temperatura óptima';
        coolingTime.textContent = '';
    }
}

// Función para actualizar los datos
async function updateData() {
    try {
        const response = await fetch('/data');
        const data = await response.json();
        
        // Actualizar temperatura
        document.getElementById('temperature').textContent = 
            formatTemperature(data.temperature);
        
        // Actualizar cantidad
        document.getElementById('quantity').textContent = 
            formatQuantity(data.quantity);
        
        // Actualizar estado de enfriamiento
        updateCoolingStatus(data.needsCooling, data.coolingTimeRemaining);
        
    } catch (error) {
        console.error('Error al obtener datos:', error);
    }
}

// Actualizar datos cada 5 segundos
setInterval(updateData, 5000);

// Primera actualización al cargar la página
updateData();