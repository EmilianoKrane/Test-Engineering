import React from 'react';
import ReactDOM from 'react-dom/client';
import { BrowserRouter, Routes, Route } from 'react-router-dom';

import MenuPrincipal from './pages/MenuPrincipal';
import CargadorBaterias from './pages/Panel_UE0089_Cargador_Baterías';
import ReguladorVoltaje from './pages/Panel_AR0150_LM2596';
import ConfiguraciónCargaVariable from './pages/Panel_Control_Carga_Variable';
import SeñueloCarga from './pages/Panel_AR4353_Señuelo _Carga';

import Ecosistema from './pages/EcosistemaUNIT.tsx';
import Proyectos from './pages/Proyectos.tsx';

import ModuloRele from './pages/Panel_UE0082_Módulo_Relé';


import TinyESP32 from './pages/Proyecto_TinyESP32.tsx';





import './index.css';


ReactDOM.createRoot(document.getElementById('root')!).render(
  <React.StrictMode>
    <BrowserRouter>
      <Routes>
        <Route path="/" element={<MenuPrincipal />} />
        <Route path="/CargadorBaterias" element={<CargadorBaterias />} />
        <Route path="/ReguladorVoltaje" element={<ReguladorVoltaje />} />
        <Route path="/CargaVariable" element={<ConfiguraciónCargaVariable />} />
        <Route path="/SeñueloCarga" element={<SeñueloCarga />} />

        <Route path="/EcosistemaUNIT" element={<Ecosistema />} />
        <Route path="/Proyectos" element={<Proyectos />} />

        <Route path="/ModuloRele" element={<ModuloRele />} />
        <Route path="/tiny ESP32" element={<TinyESP32 />} />


      
      </Routes>
    </BrowserRouter>
  </React.StrictMode>
);
