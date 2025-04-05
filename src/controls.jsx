import { useState, useEffect } from "react";
import "./App.css";
import WebSocketComponent from "./server";

function DataVisualizer() {
  const [values, setValues] = useState({ Pinch: 0, Pitch: 0, Roll: 0, Yaw: 0 });
  const data = WebSocketComponent();

  console.log("WebSocketComponent Data:", data);
  if(data?.Instruments) console.log(true);
  else console.log(false);
  
  useEffect(() => {
    if (data && data.Instruments) {
      console.log("Instruments Data:", data.Instruments);
      
      // ✅ Step 2: Update state only if data changes
      setValues((prev) => {
        return { ...prev, ...data.Instruments };
      });
    }
  }, [JSON.stringify(data?.Instruments)]); // ✅ Ensures React detects updates

  const handleChange = (param, newValue) => {
    const updatedValues = { ...values, [param]: newValue };
    setValues(updatedValues);

    if (data?.ws && data.ws.readyState === WebSocket.OPEN) {
      data.ws.send(
        JSON.stringify({ type: "updateWriteData", instrumentId: param, value: newValue })
      );
    }
  };

  return (
    <>
      <h2>Instrument Controls</h2>
      <div className="container">
        {Object.entries(values).map(([key, value]) => (
          <div key={key} className="demo">
            <div className="range-slider">
              <span className="range-label">
                <span>{key}</span>
                <span>
                  <input
                    type="range"
                    value={value}
                    min="-180"
                    max="180"
                    range="true"
                    onChange={(e) => handleChange(key, Number(e.target.value))}
                  />
                </span>
                <span className="range-value">{value}°</span>
              </span>
            </div>
          </div>
        ))}
      </div>
    </>
  );
}

export default DataVisualizer;
