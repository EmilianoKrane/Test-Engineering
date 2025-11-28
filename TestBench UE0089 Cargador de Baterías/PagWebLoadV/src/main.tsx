// src/main.tsx
import { StrictMode } from "react";
import { createRoot } from "react-dom/client";
import { BrowserRouter, Routes, Route } from "react-router-dom";
import "./index.css";

import App from "./App";          
import Config from "./components/Config";  

createRoot(document.getElementById("root")!).render(
  <StrictMode>
    <BrowserRouter>
      <Routes>
        <Route path="/" element={<App />} />          {/* App muestra Content */}
        <Route path="/config" element={<Config />} /> {/* Configuración */}
      </Routes>
    </BrowserRouter>
  </StrictMode>
);
