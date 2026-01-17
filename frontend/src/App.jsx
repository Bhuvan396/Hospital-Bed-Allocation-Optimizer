import { useEffect, useState } from "react";
import "./App.css";

function App() {
  const [beds, setBeds] = useState([]);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    fetch("/beds.json")
      .then((res) => {
        if (!res.ok) {
          throw new Error("Failed to load beds.json");
        }
        return res.json();
      })
      .then((data) => {
        setBeds(data.beds || []);
        setLoading(false);
      })
      .catch((err) => {
        console.error("Error loading JSON:", err);
        setLoading(false);
      });
  }, []);

  const getStatusClass = (occupied) => {
    return occupied ? "occupied" : "free";
  };

  if (loading) {
    return <h3 style={{ textAlign: "center" }}>Loading beds...</h3>;
  }

  return (
    <div className="container">
      <h2>Hospital Bed Visualization</h2>

      <div className="grid">
        {beds.map((bed) => (
          <div
            key={bed.id}
            className={`bed ${getStatusClass(bed.occupied)}`}
            title={`Bed ${bed.id} | Type ${bed.type} | ${
              bed.occupied ? "Occupied" : "Free"
            }`}
          >
            {bed.id}
          </div>
        ))}
      </div>
    </div>
  );
}

export default App;
