import { useEffect, useRef, useState } from "react";

const WebSocketComponent = () => {
  const [data, setData] = useState(null);
  const ws = useRef(null);

  useEffect(() => {
    const connectWebSocket = () => {
      if (ws.current) return;

      ws.current = new WebSocket("ws://localhost:5002");

      ws.current.onopen = () => {
        console.log("✅ Connected to C++ WebSocket server");
      };

      ws.current.onmessage = (event) => {
        try {
          const jsonData = JSON.parse(event.data);
          setData(jsonData);
          console.log("📩 WebSocket Data Received:", jsonData);
        } catch (error) {
          console.error("❌ Error parsing WebSocket message", error);
        }
      };

      ws.current.onclose = () => {
        console.warn("⚠️ WebSocket Disconnected. Reconnecting...");
        ws.current = null;
        setTimeout(connectWebSocket, 3000);
      };
    };

    connectWebSocket();

    return () => {
      if (ws.current) {
        ws.current.close();
        ws.current = null;
      }
    };
  }, []);

  return data; // ⬅️ Return the actual object, NOT JSON.stringify
};

export default WebSocketComponent;
