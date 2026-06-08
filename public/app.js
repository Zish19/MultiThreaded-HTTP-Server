// MultiThreaded HTTP Server - Dashboard
(function () {
    "use strict";

    const POLL_INTERVAL_MS = 3000;
    const startTime = Date.now();

    // --- Health Check ---
    async function checkHealth() {
        const indicator = document.getElementById("health-indicator");
        const statusText = document.getElementById("health-status");
        try {
            const res = await fetch("/health");
            if (res.ok) {
                indicator.className = "status-indicator online";
                statusText.textContent = "Online";
            } else {
                indicator.className = "status-indicator offline";
                statusText.textContent = "Error " + res.status;
            }
        } catch {
            indicator.className = "status-indicator offline";
            statusText.textContent = "Unreachable";
        }
    }

    // --- Metrics Polling ---
    async function fetchMetrics() {
        try {
            const res = await fetch("/metrics");
            if (!res.ok) return;
            const data = await res.json();
            setMetric("total-requests", data.total_requests);
            setMetric("cache-hits", data.cache_hits);
            setMetric("cache-misses", data.cache_misses);
            setMetric("active-connections", data.active_connections);
        } catch {
            // silently ignore fetch errors
        }
    }

    function setMetric(id, value) {
        const el = document.getElementById(id);
        if (el && value !== undefined) {
            el.textContent = formatNumber(value);
        }
    }

    function formatNumber(n) {
        if (typeof n !== "number") return String(n);
        if (n >= 1000000) return (n / 1000000).toFixed(1) + "M";
        if (n >= 1000) return (n / 1000).toFixed(1) + "K";
        return String(n);
    }

    // --- Uptime Counter ---
    function updateUptime() {
        const seconds = Math.floor((Date.now() - startTime) / 1000);
        const el = document.getElementById("uptime-value");
        if (!el) return;
        if (seconds < 60) {
            el.textContent = seconds + "s";
        } else if (seconds < 3600) {
            el.textContent = Math.floor(seconds / 60) + "m " + (seconds % 60) + "s";
        } else {
            const h = Math.floor(seconds / 3600);
            const m = Math.floor((seconds % 3600) / 60);
            el.textContent = h + "h " + m + "m";
        }
    }

    // --- Init ---
    function init() {
        checkHealth();
        fetchMetrics();
        setInterval(checkHealth, POLL_INTERVAL_MS);
        setInterval(fetchMetrics, POLL_INTERVAL_MS);
        setInterval(updateUptime, 1000);
    }

    if (document.readyState === "loading") {
        document.addEventListener("DOMContentLoaded", init);
    } else {
        init();
    }
})();
