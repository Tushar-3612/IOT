class SmartVentilationDashboard {
    constructor() {
        this.esp32IP = '';
        this.data = {};
        this.chart = null;
        this.updateInterval = null;
        
        this.chartData = {
            labels: [],
            datasets: [
                { 
                    label: 'Temperature (°C)', 
                    data: [], 
                    borderColor: '#ff6b6b', 
                    backgroundColor: 'rgba(255, 107, 107, 0.1)',
                    tension: 0.4
                },
                { 
                    label: 'Humidity (%)', 
                    data: [], 
                    borderColor: '#74b9ff', 
                    backgroundColor: 'rgba(116, 185, 255, 0.1)',
                    tension: 0.4
                },
                { 
                    label: 'MQ135', 
                    data: [], 
                    borderColor: '#55efc4', 
                    backgroundColor: 'rgba(85, 239, 196, 0.1)',
                    tension: 0.4
                },
                { 
                    label: 'MQ6', 
                    data: [], 
                    borderColor: '#ffeaa7', 
                    backgroundColor: 'rgba(255, 234, 167, 0.1)',
                    tension: 0.4
                }
            ]
        };
        
        this.init();
    }

    init() {
        this.initChart();
        this.setupEventListeners();
        this.loadSavedIP();
    }

    setupEventListeners() {
        document.getElementById('connect-btn').addEventListener('click', () => this.connectToESP32());
        document.getElementById('refresh-btn').addEventListener('click', () => this.fetchData());
        document.getElementById('auto-mode').addEventListener('click', () => this.setAutoMode());
        document.getElementById('set-fan').addEventListener('click', () => this.setManualFan());
        document.getElementById('reset-mode').addEventListener('click', () => this.resetToAuto());
        
        // Enter key support for IP input
        document.getElementById('esp32-ip').addEventListener('keypress', (e) => {
            if (e.key === 'Enter') this.connectToESP32();
        });
    }

    loadSavedIP() {
        const savedIP = localStorage.getItem('esp32IP');
        if (savedIP) {
            document.getElementById('esp32-ip').value = savedIP;
        }
    }

    saveIP() {
        localStorage.setItem('esp32IP', this.esp32IP);
    }

    async connectToESP32() {
        const ipInput = document.getElementById('esp32-ip').value.trim();
        if (!ipInput) {
            alert('Please enter ESP32 IP address');
            return;
        }

        this.esp32IP = ipInput;
        this.saveIP();

        // Test connection
        this.updateConnectionStatus('connecting', 'Testing connection...');
        
        try {
            const response = await fetch(`http://${this.esp32IP}/data`, { 
                method: 'GET',
                timeout: 5000 
            });
            
            if (response.ok) {
                this.updateConnectionStatus('connected', `Connected to ${this.esp32IP}`);
                this.startAutoUpdate();
                this.fetchData(); // First data fetch
            } else {
                throw new Error('Connection failed');
            }
        } catch (error) {
            this.updateConnectionStatus('error', `Connection failed: ${error.message}`);
            console.error('Connection error:', error);
        }
    }

    updateConnectionStatus(status, message) {
        const statusDot = document.getElementById('status-dot');
        const statusText = document.getElementById('connection-status');
        
        statusDot.className = 'dot';
        statusText.textContent = message;

        switch (status) {
            case 'connected':
                statusDot.classList.add('connected');
                break;
            case 'error':
                statusDot.style.background = '#ff4757';
                break;
            case 'connecting':
                statusDot.style.background = '#ffa502';
                break;
        }
    }

    startAutoUpdate() {
        // Clear existing interval
        if (this.updateInterval) {
            clearInterval(this.updateInterval);
        }
        
        // Update every 2 seconds
        this.updateInterval = setInterval(() => this.fetchData(), 2000);
    }

    async fetchData() {
        if (!this.esp32IP) {
            this.updateConnectionStatus('error', 'Not connected to ESP32');
            return;
        }

        try {
            const response = await fetch(`http://${this.esp32IP}/data`);
            if (!response.ok) {
                throw new Error(`HTTP error! status: ${response.status}`);
            }
            
            const data = await response.json();
            this.data = data;
            this.data.timestamp = new Date().toLocaleTimeString();
            
            this.updateDashboard();
            this.updateChart();
            this.updateConnectionStatus('connected', `Connected to ${this.esp32IP}`);
            
        } catch (error) {
            console.error('Error fetching data:', error);
            this.updateConnectionStatus('error', `Connection lost: ${error.message}`);
        }
    }

    updateDashboard() {
        // Update temperature
        if (this.data.temperature !== null && this.data.temperature !== undefined) {
            document.getElementById('temp-value').textContent = this.data.temperature.toFixed(1);
            const status = this.data.temperature > 30 ? 'High' : 'Normal';
            document.getElementById('temp-status').textContent = status;
            document.getElementById('temp-status').style.background = 
                this.data.temperature > 30 ? 'rgba(255, 107, 107, 0.5)' : 'rgba(46, 213, 115, 0.5)';
        }

        // Update humidity
        if (this.data.humidity !== null && this.data.humidity !== undefined) {
            document.getElementById('hum-value').textContent = this.data.humidity.toFixed(1);
            const status = this.data.humidity > 70 ? 'High' : 'Normal';
            document.getElementById('hum-status').textContent = status;
            document.getElementById('hum-status').style.background = 
                this.data.humidity > 70 ? 'rgba(255, 107, 107, 0.5)' : 'rgba(46, 213, 115, 0.5)';
        }

        // Update MQ135
        if (this.data.mq135 !== null && this.data.mq135 !== undefined) {
            document.getElementById('mq135-value').textContent = this.data.mq135;
            let status = 'Normal';
            let color = 'rgba(46, 213, 115, 0.5)';
            
            if (this.data.mq135 > 500) {
                status = 'Danger';
                color = 'rgba(255, 107, 107, 0.5)';
            } else if (this.data.mq135 > 400) {
                status = 'Warning';
                color = 'rgba(255, 165, 2, 0.5)';
            }
            
            document.getElementById('mq135-status').textContent = status;
            document.getElementById('mq135-status').style.background = color;
        }

        // Update MQ6
        if (this.data.mq6 !== null && this.data.mq6 !== undefined) {
            document.getElementById('mq6-value').textContent = this.data.mq6;
            let status = 'Normal';
            let color = 'rgba(46, 213, 115, 0.5)';
            
            if (this.data.mq6 > 500) {
                status = 'Danger';
                color = 'rgba(255, 107, 107, 0.5)';
            } else if (this.data.mq6 > 400) {
                status = 'Warning';
                color = 'rgba(255, 165, 2, 0.5)';
            }
            
            document.getElementById('mq6-status').textContent = status;
            document.getElementById('mq6-status').style.background = color;
        }

        // Update motion
        if (this.data.motion !== null && this.data.motion !== undefined) {
            const motionText = this.data.motion ? 'Motion Detected' : 'No Motion';
            const motionColor = this.data.motion ? 'rgba(46, 213, 115, 0.5)' : 'rgba(255, 107, 107, 0.5)';
            
            document.getElementById('motion-value').textContent = this.data.motion ? 'YES' : 'NO';
            document.getElementById('motion-status').textContent = motionText;
            document.getElementById('motion-status').style.background = motionColor;
        }

        // Update fan
        if (this.data.fanSpeed !== null && this.data.fanSpeed !== undefined) {
            const fanSpeed = this.data.fanSpeed;
            document.getElementById('fan-speed-value').textContent = fanSpeed;
            document.getElementById('fan-display').textContent = fanSpeed;
            document.getElementById('fan-slider').value = fanSpeed;
            document.getElementById('fan-progress').style.width = `${(fanSpeed / 255) * 100}%`;
            
            // Update fan status
            let fanStatus = 'Auto Mode';
            if (this.data.manualOverride) {
                fanStatus = 'Manual Mode';
            } else if (fanSpeed === 255) {
                fanStatus = 'High Speed';
            } else if (fanSpeed === 150) {
                fanStatus = 'Medium Speed';
            } else if (fanSpeed === 0) {
                fanStatus = 'Stopped';
            }
            
            document.getElementById('fan-status').textContent = fanStatus;
        }

        // Update system info
        this.updateSystemInfo();
    }

    updateSystemInfo() {
        const infoDiv = document.getElementById('system-info');
        let infoHTML = `
            <p><strong>Last Update:</strong> ${this.data.timestamp}</p>
            <p><strong>Device IP:</strong> ${this.data.deviceIP || 'Unknown'}</p>
            <p><strong>Mode:</strong> ${this.data.manualOverride ? 'Manual' : 'Auto'}</p>
        `;
        infoDiv.innerHTML = infoHTML;
    }

    initChart() {
        const ctx = document.getElementById('sensorChart').getContext('2d');
        this.chart = new Chart(ctx, {
            type: 'line',
            data: this.chartData,
            options: {
                responsive: true,
                maintainAspectRatio: false,
                interaction: {
                    mode: 'index',
                    intersect: false,
                },
                scales: {
                    y: {
                        beginAtZero: true,
                        grid: {
                            color: 'rgba(255, 255, 255, 0.1)'
                        },
                        ticks: {
                            color: 'white'
                        }
                    },
                    x: {
                        grid: {
                            color: 'rgba(255, 255, 255, 0.1)'
                        },
                        ticks: {
                            color: 'white'
                        }
                    }
                },
                plugins: {
                    legend: {
                        labels: {
                            color: 'white'
                        }
                    }
                }
            }
        });
    }

    updateChart() {
        const now = new Date().toLocaleTimeString();
        
        // Limit to last 20 data points
        if (this.chartData.labels.length > 20) {
            this.chartData.labels.shift();
            this.chartData.datasets.forEach(dataset => dataset.data.shift());
        }

        this.chartData.labels.push(now);
        this.chartData.datasets[0].data.push(this.data.temperature);
        this.chartData.datasets[1].data.push(this.data.humidity);
        this.chartData.datasets[2].data.push(this.data.mq135);
        this.chartData.datasets[3].data.push(this.data.mq6);

        this.chart.update('none');
    }

    async setManualFan() {
        const fanSpeed = document.getElementById('fan-slider').value;
        
        if (!this.esp32IP) {
            alert('Not connected to ESP32');
            return;
        }

        try {
            const response = await fetch(`http://${this.esp32IP}/set?fan=${fanSpeed}`);
            if (response.ok) {
                this.fetchData(); // Refresh data
            } else {
                throw new Error('Failed to set fan speed');
            }
        } catch (error) {
            console.error('Error setting fan speed:', error);
            alert('Error setting fan speed: ' + error.message);
        }
    }

    async setAutoMode() {
        if (!this.esp32IP) {
            alert('Not connected to ESP32');
            return;
        }

        try {
            const response = await fetch(`http://${this.esp32IP}/reset`);
            if (response.ok) {
                this.fetchData(); // Refresh data
            } else {
                throw new Error('Failed to set auto mode');
            }
        } catch (error) {
            console.error('Error setting auto mode:', error);
            alert('Error setting auto mode: ' + error.message);
        }
    }

    async resetToAuto() {
        await this.setAutoMode();
    }
}

// Initialize dashboard when page loads
document.addEventListener('DOMContentLoaded', () => {
    window.dashboard = new SmartVentilationDashboard();
});