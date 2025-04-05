import React, { useState } from "react";
import RealTimeChart from "./RealTimeChart";
import "./Plot.css";
import WebSocketComponent from "./server"; // Importing your WebSocket component

const Plot = () => {
  const [activeSection, setActiveSection] = useState(null);
  const [instrumentId, setInstrumentId] = useState(null);

  const data = WebSocketComponent(); // Fetching real-time data

  return (
    <div className="Ui">
      <div className="cla">
        <h2>Actuators</h2>
        <div className="button-box">
          {[1, 2, 3, 4].map((id) => (
            <button
              key={id}
              onClick={() => {
                setInstrumentId(id);
                setActiveSection(`Actuator ${id}`);
              }}
              className={activeSection === `Actuator ${id}` ? "active" : ""}
            >
              Actuator {id}
            </button>
          ))}
        </div>
      </div>

      <div className="plot">
        {instrumentId !== null && data?.Actuators && (
          <RealTimeChart
            key={instrumentId}
            instrumentId={instrumentId}
            ws={data?.ws} // Pass WebSocket connection
            instrumentData={data?.Actuators} // Pass real-time actuator data
          />
        )}
      </div>
    </div>
  );
};

export default Plot;
