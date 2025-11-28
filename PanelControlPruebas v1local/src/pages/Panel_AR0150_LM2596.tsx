import React, { useState, useEffect, useRef } from "react";
import Header from "../components/Header";
import Footer from "../components/Footer";
import { Link } from 'react-router-dom';

export default function Content() {

  // Estados para los LEDs
  const [stateLED1, setLED1On] = useState("disp");
  const [stateLED2, setLED2On] = useState("disp");

  // Estado de Observaciones
  //const [observaciones, setObservaciones] = useState("");


  // Estado para el contenido de la caja de texto para ID 
  const [IDProducto, setIDProducto] = useState<string>("");
  const [autoIncrement, setAutoIncrement] = useState(false); // checkbox
  let nuevoIDTestBench: number | null = null;

  // Referencia al input 
  const inputRef = useRef<HTMLInputElement>(null);

  // Estado para la comunicación serie por puerto
  const [port1, setPort1] = useState<SerialPort | null>(null);
  const [port2, setPort2] = useState<SerialPort | null>(null);

  // Estado para el aviso de finalizado
  const [showMessage, setShowMessage] = useState(false);
  const [showMessageError, setShowMessageError] = useState(false);

  useEffect(() => {
    if (stateLED1 == "on" && stateLED2 == "on") {
      setShowMessage(true);   // Mostrar el aviso
    }
  }, [stateLED1, stateLED2]);

  useEffect(() => {
    if (stateLED1 == "off" || stateLED2 == "off") {
      setShowMessageError(true);   // Mostrar el aviso de error
    }
  }, [stateLED1, stateLED2]);


  // Cuando el componente carga, ponemos el foco en el input
  useEffect(() => {
    inputRef.current?.focus();
  }, []);


  // Función para esperar N milisegundos
  const delay = (ms: number) => new Promise(resolve => setTimeout(resolve, ms));

  const predefinedComments = [
    "No demanda corriente, no enciende LED",
    "No demanda corriente, enciende LED",
    "El LED enciende muy tenue",
    "El voltaje se cae",
    "Tiene desplazado un componente",
    "Calentamiento y poca corriente después de corto ",
  ];


  const [observaciones, setObservaciones] = useState("");
  const [showDropdown, setShowDropdown] = useState(false);

  const handleSelect = (comment: string) => {
    setObservaciones(comment);
    setShowDropdown(false);
  };

  // Manejar cuando el GM65 “escribe” algo
  const handleChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    setIDProducto(e.target.value); // copiar lo que se escribió al input
  };

  // Manejar cuando el GM65 manda "Enter" después del código
  const handleKeyDown = (e: React.KeyboardEvent<HTMLInputElement>) => {
    if (e.key === "Enter") {
      console.log("Código leído:", e.currentTarget.value);

      // Mostrar en el input y limpiar valor
      //e.currentTarget.value = "";
      setIDProducto(e.currentTarget.value);

      // Esperar 30 segundos antes de borrar el input
      setTimeout(() => {
        setIDProducto("");
      }, 30000); // 10000ms = 10 segundos

    }
  };

  const getColor1 = () => {
    switch (stateLED1) {
      case "on":
        return "limegreen"; // Enciende con 1
      case "waiting":
        return "yellow";    // Enciende con 2
      case "off":
        return "#f73d18ff"; // Enciende con 0
      default:
        return "#a193ff77";
    }
  };

  const getColor2 = () => {
    switch (stateLED2) {
      case "on":
        return "limegreen"; // Enciende con 1
      case "waiting":
        return "yellow";    // Enciende con 2
      case "off":
        return "#f73d18ff"; // Enciende con 0
      default:
        return "#a193ff77";
    }
  };



  const connectDUALONE = async () => {
    try {
      // pedir puerto al usuario
      const selectedPort: SerialPort = await (navigator as any).serial.requestPort();
      await selectedPort.open({ baudRate: 115200 });

      // guardamos el puerto en el estado
      setPort1(selectedPort);
      console.log("Puerto COM Test conectado ");

      // empezamos a leer de Arduino
      readFromDual(selectedPort);
    } catch (err) {
      console.error("Error al conectar con la placa:", err);
    }
  };

  const connectCOMLoad = async () => {
    try {
      // pedir puerto al usuario
      const selectedPort: SerialPort = await (navigator as any).serial.requestPort();
      await selectedPort.open({ baudRate: 9600 });

      // guardamos el puerto en el estado
      setPort2(selectedPort);
      console.log("Puerto COM CargaV conectado");

      // empezamos a leer de la carga
      readFromLoad(selectedPort);
    } catch (err) {
      console.error("Error al conectar con la placa:", err);
    }
  };


  const sendToPulsar = async (char: string) => {
    if (port1 && port1.writable) {
      const encoder = new TextEncoder();
      const writer = port1.writable.getWriter();
      await writer.write(encoder.encode(char));
      writer.releaseLock();
      console.log("JSON enviado a PULSAR:", char); // <-- ahora sí imprime el dato enviado
    }
  };

  const sendToLoad = async (char: string) => {
    if (port2 && port2.writable) {
      const encoder = new TextEncoder();
      const writer = port2.writable.getWriter();
      await writer.write(encoder.encode(char + "\r\n"));
      writer.releaseLock();
      console.log("✅ JSON enviado:", char);
    }
  };

  const readFromDual = async (selectedPort: SerialPort) => {
    const decoder = new TextDecoderStream();
    (selectedPort.readable as any).pipeTo(decoder.writable as any);
    const reader = decoder.readable.getReader();

    let buffer = "";

    while (true) {
      const { value, done } = await reader.read();
      if (done) break;

      if (value) {
        buffer += value; // acumulamos en el buffer

        let parts = buffer.split("\n");  // separamos por salto de línea
        buffer = parts.pop() || "";      // guardamos lo que quedó incompleto

        for (const part of parts) {

          const trimmed = part.trim();
          if (!trimmed) continue; //evita parsear vacío

          try {
            const parsed = JSON.parse(trimmed);
            console.log("JSON recibido:", parsed); // Imprimes JSON en crudo

            if (parsed) {
              // 🔔 Emitimos evento con el JSON recibido
              window.dispatchEvent(new CustomEvent("jsonReceived", { detail: parsed }));
              if (parsed.Led1) setLED1On(parsed.Led1);
              if (parsed.Led2) setLED2On(parsed.Led2);
            }
          }
          catch (err) {
            console.error("Error parseando JSON:", err, trimmed);
          }
        }
      }
    }
  };


  const readFromLoad = async (selectedPort: SerialPort) => {
    const decoder = new TextDecoderStream();
    (selectedPort.readable as any).pipeTo(decoder.writable as any);
    const reader = decoder.readable.getReader();

    let buffer = "";

    while (true) {
      const { value, done } = await reader.read();
      if (done) break;

      if (value) {
        buffer += value; // acumulamos en el buffer

        let parts = buffer.split("\n");  // separamos por salto de línea
        buffer = parts.pop() || "";      // guardamos lo que quedó incompleto

        for (const part of parts) {

          const trimmed = part.trim();
          if (!trimmed) continue; //evita parsear vacío

          try {
            const parsed = JSON.parse(trimmed);
            console.log("JSON recibido:", parsed); // Imprimes JSON en crudo
            if (parsed) {
              window.dispatchEvent(new CustomEvent("jsonReceived", { detail: parsed }));
            }
          }
          catch (err) {
            console.error("Error parseando JSON:", err, trimmed);
          }
        }
      }
    }
  };


  // Función de envío de ID a la Base de Datos
  const enviarIDProducto = async (ID_Producto: string) => {
    const JSON_ID = {
      id_proyecto: 118,     // ID de Proyecto fijo
      id_mac: "",
      uid: "",
      id_numero_serie: ID_Producto,
      id_tecnico: 1,
      comentarios_generales: "Prueba de LM2596",
    };

    console.log("El ID enviado es: ", ID_Producto);
    const resultado = await enviarNewTestBench(JSON_ID);
    await delay(2000);

    if (resultado.success && resultado.id_testBench) {
      console.log("✅ Datos de ID enviados correctamente");


      nuevoIDTestBench = resultado.id_testBench;
      console.log("El ID de la PCB es: ", nuevoIDTestBench);

      return true;
    } else {
      alert("❌ Falló el envío de ID a la base de datos");
      return false;
    }
  };

  // Función para enviar ID NewTestBench a la Base de Datos
  type TestBenchResponse = {
    success: boolean;
    id_testBench?: number; // opcional, solo si existe
  };

  const enviarNewTestBench = async (
    jsonData: Record<string, any>
  ): Promise<TestBenchResponse> => {
    try {
      const response = await fetch("/api/BoardTesting/setNewTestBench", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(jsonData),
      });

      if (!response.ok) {
        console.error("❌ Error en la respuesta:", response.status, response.statusText);
        return { success: false };
      }

      // ✅ Parseo de JSON de respuesta
      const dataresponse = await response.json();
      console.log("Respuesta del servidor:", dataresponse);

      const id_testBench = dataresponse?.data?.[0]?.id_testBench ?? undefined;

      if (id_testBench) {
        console.log("El id_setPruebas es:", id_testBench);
        return { success: true, id_testBench };
      } else {
        console.warn("⚠️ No se encontró id_testBench en la respuesta");
        return { success: true }; // enviado bien, pero sin id
      }
    } catch (err) {
      console.error("❌ Error enviando JSON:", err);
      return { success: false };
    }
  };

  // Función para enviar nueva prueba NewTest a la base de datos 
  const enviarNewTest = async (statusPrueba: number,
    tipoPrueba: number): Promise<boolean> => {

    let JSON_Test = {
      "id_setPruebas": nuevoIDTestBench,
      "id_tipo_prueba": tipoPrueba,
      "id_status_prueba": statusPrueba,  // 1: Prueba OK  2: Prueba Falló
      "comentarios": observaciones,
      "parametro_1": "",
      "parametro_2": "",
      "parametro_3": ""
    };


    try {
      const response = await fetch("/api/BoardTesting/setNewTest", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(JSON_Test),
      });

      await delay(100);
      if (!response.ok) {
        console.error("❌ Error en la respuesta:", response.status, response.statusText);
        return false;
      }

      const data = await response.json();
      console.log("✅ Enviado correctamente:", data);
      return true;

    } catch (err) {
      console.error("❌ Error enviando JSON:", err);
      return false;
    }
  };






  // --- Espera una respuesta JSON con "Result":"OK" ---
  const waitForOK = async (): Promise<boolean> => {
    return new Promise((resolve) => {
      const timeout = setTimeout(() => {
        resolve(false); // ⏱️ No llegó OK en 2s
      }, 3000);

      const checkResponse = (event: CustomEvent) => {
        const parsed = event.detail;

        if (parsed.Result === "OK") {
          clearTimeout(timeout);
          window.removeEventListener("jsonReceived", checkResponse as any);
          resolve(true); // ✅ OK recibido
        }
      };
      window.addEventListener("jsonReceived", checkResponse as any);
    });
  };


  // --- Secuencia completa ---
  const Arranque = async () => {
    const start = Date.now(); // tiempo inicial

    if (!port1) {
      alert("Selecciona un puerto COM para el controlador.");
      return;
    }

    if (!port2) {
      alert("Selecciona un puerto COM para la carga variable");
      return;
    }
        

    try {
      const exito = await enviarIDProducto(String(IDProducto));
      if (!exito) return;

      setLED1On("waiting");       // Espera LED1 Prueba Cortocircuito
      setLED2On("waiting");       // Espera LED2 Prueba Demanda Corriente

      // ===== Rutina de Prueba de Cortocircuito =====
      await runTestNominal();

      // ===== Rutina de Prueba de Carga Variable =====
      await runCortocircuito();

      console.log("✅ Secuencia completada correctamente");
      const end = Date.now(); // tiempo final
      const elapsed = (end - start) / 1000; // en segundos

      console.log(`✅ Finalizado en ${elapsed.toFixed(2)} segundos`);

    // --- Lógica del checkbox y autoincremento ---
    if (autoIncrement) {
      // Separar prefijo y número
      const prefix = IDProducto.match(/^[^\d]+/)?.[0] || "";
      const numberPart = IDProducto.match(/\d+$/)?.[0] || "0";

      // Incrementar el número y mantener ceros a la izquierda
      const nextNumber = String(Number(numberPart) + 1).padStart(numberPart.length, "0");

      const nextID = prefix + nextNumber;
      setIDProducto(nextID);

      if (inputRef.current) {
        inputRef.current.value = nextID;
      }
    }


    } catch (err) {
      console.error("❌ Error en la secuencia:", err);
    }
  };


  async function runTestNominal() {
    try {
      await delay(500);
      console.log("➡️ Configurando la carga a 3A...");
      await sendToLoad('{"Funcion": "DYN", "Config": {"Resolution": [3.1, 1, 3.1, 1], "Range": [50, 30]}, "Start": "CFG_ON"}');
      await delay(1000);

      console.log("➡️ Encendiendo la carga...");
      await sendToLoad('{"Funcion": "Other", "Config": {"Resolution": [], "Range": []}, "Start": "CFG_ON"}');
      await delay(1000);

      console.log("➡️ Midiendo demanda nominal...");
      await sendToPulsar(JSON.stringify({ Function: "Lectura Nom" }) + "\n");
      let OKDemanda = await waitForOK(); // Confirmación de demanda 3A
      if (OKDemanda) {
        console.log("✅ Demanda Nominal corto OK");
        setLED1On("on");       // Ok demanda nominal

        // enviarNewTest(statusPrueba, idPrueba)
        // statusPrueba: 1 -> OK | 2 -> Falla
        // idPrueba: 25 Carga de Batería
        const exito_Test = await enviarNewTest(1, 18);

        if (exito_Test) {
          console.log("✅ Datos de Test enviados correctamente");
        } else {
          alert("❌ Falló el envío del test a la Base de Datos, favor de repetir prueba.");
          await sendToLoad('{"Funcion": "Other", "Config": {"Resolution": [], "Range": []}, "Start": "CFG_OFF"}');
          return;
        }
      }
      else {
        console.log("❌ Falló demanda nominal");
        setLED1On("off");       // Error demanda nominal
        await sendToLoad('{"Funcion": "Other", "Config": {"Resolution": [], "Range": []}, "Start": "CFG_OFF"}');
        await delay(500);

        // enviarNewTest(statusPrueba, idPrueba)
        // statusPrueba: 1 -> OK | 2 -> Falla
        // idPrueba: 25 Carga de Batería
        const exito_Test = await enviarNewTest(2, 18);

        if (exito_Test) {
          console.log("✅ Datos de Test enviados correctamente");
        } else {
          alert("❌ Falló el envío del test a la Base de Datos, favor de repetir prueba.");
          return;
        }

        return;
      }

    } catch (error) {
      console.error("⚠️ Error en rutina:", error);
      setLED1On("off");
      setLED2On("off");
      await delay(500);
      await sendToLoad('{"Funcion": "Other", "Config": {"Resolution": [], "Range": []}, "Start": "CFG_OFF"}');
    }
  }


  // ⚡ Función para ejecutar la rutina de Carga Variable
  async function runCortocircuito() {
    try {
      // Inicio de Prueba de Corto
      console.log("➡️ Enviando Cortocircuito...");
      await delay(100);
      await sendToPulsar(JSON.stringify({ Function: "Cortocircuito" }) + "\n");
      const OKCorto = await waitForOK(); // Confirmación de demanda 0A
      if (OKCorto) {
        console.log("✅ Protección en corto OK");
      }
      else {
        console.log("❌ Falló la protección de corto");
        setLED2On("off");       // Error demanda corto
        await delay(500);
        await sendToLoad('{"Funcion": "Other", "Config": {"Resolution": [], "Range": []}, "Start": "CFG_OFF"}');

        // statusPrueba: 1 -> OK | 2 -> Falla
        // idPrueba: 15 Cortocircuito
        let exito = await enviarNewTest(2, 15);
        if (exito) {
          console.log("✅ Datos de Test de corto enviados correctamente");
        } else {
          alert("❌ Falló el envío del test de corto a la Base de Datos, favor de repetir prueba.");
        }

        return;
      }

      await delay(2000);
      console.log("➡️ Midiendo demanda...");
      await sendToPulsar(JSON.stringify({ Function: "Lectura Nom" }) + "\n");
      const OKDemandaC2 = await waitForOK(); // Confirmación de demanda 3A
      if (OKDemandaC2) {
        console.log("✅ Demanda después de corto OK");
        setLED2On("on");

        // Prueba aprobada
        // statusPrueba: 1 -> OK | 2 -> Falla
        // idPrueba: 15 Cortocircuito
        let exito = await enviarNewTest(1, 15);

        if (exito) {
          console.log("✅ Datos de Test de corto enviados correctamente");
        } else {
          alert("❌ Falló el envío del test de corto a la Base de Datos, favor de repetir prueba.");
        }
      }

      else {
        console.log("❌ Falló demanda después de corto");
        setLED2On("off");       // Error demanda nominal
        await delay(500);
        await sendToLoad('{"Funcion": "Other", "Config": {"Resolution": [], "Range": []}, "Start": "CFG_OFF"}');

        // statusPrueba: 1 -> OK | 2 -> Falla
        // idPrueba: 15 Cortocircuito
        let exito = await enviarNewTest(2, 15);
        if (exito) {
          console.log("✅ Datos de Test de corto enviados correctamente");
        } else {
          alert("❌ Falló el envío del test de corto a la Base de Datos");
        }
      }

      console.log("➡️ Apagando la carga...");
      await sendToLoad('{"Funcion": "Other", "Config": {"Resolution": [], "Range": []}, "Start": "CFG_OFF"}');

    } catch (error) {
      console.error("⚠️ Error en rutina Cortocircuito:", error);
      setLED2On("off");
      await sendToLoad('{"Funcion": "Other", "Config": {"Resolution": [], "Range": []}, "Start": "CFG_OFF"}');
    }
  }





  return (
    <main
      style={{
        width: "565px",        // ancho fijo
        margin: "0 auto",      // centra horizontalmente
        flex: 1,
        display: "flex",
        flexDirection: "column",
        gap: "20px",
        padding: "20px",
      }}
    >

      <div style={{ position: "fixed", top: 0, left: 0, width: "100%", height: "60px", zIndex: 100 }}>
        <Header />
      </div>



      {/* Primera Sección: ID de producto */}
      <div
        style={{
          display: "flex",       // alineación en fila
          alignItems: "center",  // centrado vertical
          gap: "10px",           // espacio entre elementos
          marginTop: "140px"
        }}
      >
        <h2 style={{ margin: 0 }}>ID del producto</h2>

        <input
          ref={inputRef}
          type="text"
          value={IDProducto}
          onChange={handleChange}
          onKeyDown={handleKeyDown}
          style={{ width: "150px", height: "25px" }}
        />

        {/*COM para Pulsar Control*/}
        <button
          onClick={connectDUALONE}
          style={{
            padding: "10px",
            width: "100px",
            borderRadius: "5px",
            cursor: "pointer",
            backgroundColor: port1 ? "#4CAF50" : "#e4e4e490", // Verde si conectado
            color: port1 ? "white" : "black",
            fontWeight: port1 ? "bold" : "normal",
          }}
        >
          {port1 ? "COM TEST ✅" : "Seleccionar COM TEST"}
        </button>

        {/*COM para Pulsar Potencia*/}
        <button
          onClick={connectCOMLoad}
          style={{
            padding: "10px",
            width: "100px",
            borderRadius: "5px",
            cursor: "pointer",
            backgroundColor: port2 ? "#4CAF50" : "#e4e4e490", // Verde si conectado
            color: port2 ? "white" : "black",
            fontWeight: port2 ? "bold" : "normal",
          }}
        >
          {port2 ? "COM CVar ✅" : "Seleccionar COM CVar"}
        </button>


      </div>

      {/* Segunda Sección: Tablero de indicadores*/}
      <div
        style={{
          display: "flex",
          flexDirection: "row",   // o "row" si quieres en fila
          justifyContent: "center",  // centra verticalmente
          alignItems: "center",      // centra horizontalmente
          border: "2px solid #333",       // borde oscuro
          borderRadius: "10px",           // esquinas redondeadas
          padding: "40px",                // espacio interno
          backgroundColor: "#f5f5f5",     // color de fondo claro
          boxShadow: "2px 2px 10px rgba(0,0,0,0.2)", // sombra para dar relieve
          width: "490px",                 // ancho fijo opcional
          height: "1px",                 // ancho fijo opcional
        }}
      >

        <h3
          style={{
            alignItems: "center",
            marginTop: 20
          }}

        >Tablero de indicadores 🔎 </h3>

        {/* Configuración Fila 1 */}
        <div style={{
          display: "flex",
          alignItems: "center",
          gap: "12px"
        }}>

          <div
            style={{
              width: "20px",
              height: "20px",
              borderRadius: "50%",
              backgroundColor: "limegreen",
              border: "1px solid #333",
            }}
          ></div>
          <span>Prueba aprobada</span>
        </div>

        {/* Configuración Fila 2 */}
        <div style={{
          display: "flex",
          alignItems: "center",
          gap: "12px"
        }}>
          <div
            style={{
              width: "20px",
              height: "20px",
              borderRadius: "50%",
              backgroundColor: "red",
              border: "1px solid #333",
            }}
          ></div>
          <span>Prueba fallida</span>
        </div>

        {/* Configuración Fila 3 */}
        <div style={{
          display: "flex",
          alignItems: "center",
          gap: "12px"
        }}>
          <div
            style={{
              width: "20px",
              height: "20px",
              borderRadius: "50%",
              backgroundColor: "yellow",
              border: "1px solid #333",
            }}
          ></div>
          <span>Espera...</span>
        </div>
      </div> {/*div Final de Sección */}



      {/* Tercera Sección: Botones e indicadores LEDS */}
      <div
        style={{
          flex: 1,
          display: "flex", // dividir en columnas
          justifyContent: "center",
          alignItems: "flex-start",
          gap: "10px",
          padding: "5px",
        }}
      >

        {/* Columna izquierda: Botones de Prueba*/}
        <div
          style={{
            flex: 1,
            display: "flex",
            flexDirection: "column",
            alignItems: "center",
            gap: "10px",
          }}
        >
          <h3>PRUEBA TEST</h3>

          <h3
            style={{ margin: 2 }}
          >Corriente Nom</h3>

          <h3
            style={{ margin: 10 }}
          >Cortocircuito</h3>

        </div>

        {/* Columna derecha: LEDs indicadores */}
        <div
          style={{
            flex: 1,
            display: "flex",
            flexDirection: "column",
            justifyContent: "center",
            alignItems: "center",
            gap: "12px",
            padding: "0px",
          }}
        >
          <h3>ESTADO</h3>

          {/* LED Cortocirtuito */}
          <div
            style={{
              width: "30px",
              height: "30px",
              borderRadius: "50%",
              backgroundColor: getColor1(),
              border: "2px solid #333",
              marginBottom: "5px",
              transition: "background-color 0.5s ease" // Transición
            }}
          ></div>

          {/* LED Polaridad */}
          <div
            style={{
              width: "30px",
              height: "30px",
              borderRadius: "50%",
              backgroundColor: getColor2(),
              border: "2px solid #333",
              marginBottom: "5px",
              transition: "background-color 0.5s ease" // Transición
            }}
          ></div>

        </div>




        {/* Columna derecha: Botones de control */}
        <div style={{
          flex: 1,
          display: "flex",
          flexDirection: "column",
          justifyContent: "center",
          alignItems: "center",
          marginTop: "10px",
          gap: "10px"
        }}>

          {/* Checkbox de Auto Incremento */}
          <div
            style={{
              marginTop: "15px",
              display: "flex",
              alignItems: "center",
              gap: "10px",
              backgroundColor: "#f5f5f5",
              padding: "8px 12px",
              borderRadius: "8px",
              boxShadow: "0 2px 5px rgba(0,0,0,0.1)",
              width: "fit-content",
              cursor: "pointer",
              transition: "background-color 0.2s ease",
            }}
            onMouseEnter={(e) =>
              (e.currentTarget.style.backgroundColor = "#e0e0e0")
            }
            onMouseLeave={(e) =>
              (e.currentTarget.style.backgroundColor = "#f5f5f5")
            }
          >
            <input
              type="checkbox"
              checked={autoIncrement}
              onChange={(e) => setAutoIncrement(e.target.checked)}
              id="autoIncrement"
              style={{
                width: "18px",
                height: "18px",
                cursor: "pointer",
              }}
            />
            <label
              htmlFor="autoIncrement"
              style={{
                fontWeight: "500",
                color: "#333",
                cursor: "pointer",
                userSelect: "none",
              }}
            >
              ID automático
            </label>
          </div>


          <button
            onClick={Arranque}
            style={{
              padding: "20px 0px",
              borderRadius: "6px",
              border: "none",
              backgroundColor: "#3a26efff",
              color: "white",
              fontWeight: "bold",
              cursor: "pointer",
              boxShadow: "0 3px 6px rgba(0,0,0,0.3)",
              transition: "background-color 0.3s ease",
              height: "80px",
              width: "120px",
              marginRight: "30px",
              marginTop: "25px",
              fontSize: "26px"  // Aumento de tamaño de letra
            }}
            onMouseOver={(e) => (e.currentTarget.style.backgroundColor = "#11067aff")}
            onMouseOut={(e) => (e.currentTarget.style.backgroundColor = "#3a26efff")}
          >
            ¡Inicio!
          </button>







        </div>
      </div>



      {/* Sección de Observaciones */}
      <div
        style={{
          display: "flex",
          alignItems: "flex-start",
          gap: "10px",
          width: "100%",
          maxWidth: "500px",
          margin: "20px auto",
          padding: "20px",
          border: "1px solid #ccc",
          borderRadius: "8px",
          marginBottom: "10px",
          position: "relative", // necesario para el dropdown
        }}
      >
        <label htmlFor="observaciones" style={{ width: "120px", fontWeight: "bold" }}>
          Observaciones:
        </label>
        <div style={{ flex: 1, position: "relative" }}>
          <textarea
            id="observaciones"
            value={observaciones}
            onChange={(e) => setObservaciones(e.target.value)}
            onFocus={() => setShowDropdown(true)}
            onBlur={() => setTimeout(() => setShowDropdown(false), 150)}
            placeholder="Escribe tus observaciones aquí..."
            style={{
              width: "100%",
              minHeight: "60px",
              padding: "5px",
              resize: "vertical",
              fontSize: "1rem",
            }}
          />
          {showDropdown && (
            <ul
              style={{
                position: "absolute",
                top: "100%",
                left: 0,
                width: "100%",
                border: "1px solid #ccc",
                backgroundColor: "white",
                listStyle: "none",
                margin: 0,
                padding: 0,
                maxHeight: "150px",
                overflowY: "auto",
                zIndex: 100,
              }}
            >
              {predefinedComments.map((comment, index) => (
                <li
                  key={index}
                  onClick={() => handleSelect(comment)}
                  onMouseDown={(e) => e.preventDefault()} // evita que el blur cierre antes
                  style={{
                    padding: "8px",
                    cursor: "pointer",
                    borderBottom: "1px solid #eee",
                  }}
                >
                  {comment}
                </li>
              ))}
            </ul>
          )}
        </div>
      </div>





      <div
        style={{
          display: "flex",
          justifyContent: "center",
          alignItems: "center",
          //height: "100vh", // ocupa toda la altura de la ventana
          marginTop: "0px",
          marginBottom: "0px",
          width: "100%",   // ocupa todo el ancho
          //backgroundColor: "#f9f9f9", // opcional, un fondo claro
        }}
      >
        <Link
          to="/"
          style={{
            display: "inline-block",
            padding: "16px 32px",
            background: "linear-gradient(135deg, #667eea, #764ba2)",
            color: "white",
            fontSize: "20px",
            fontWeight: "bold",
            textDecoration: "none",
            borderRadius: "12px",
            boxShadow: "0 4px 12px rgba(0,0,0,0.25)",
            transition: "all 0.3s ease",
            textAlign: "center",
          }}
          onMouseOver={(e) => {
            e.currentTarget.style.background =
              "linear-gradient(135deg, #5a67d8, #6b46c1)";
            e.currentTarget.style.transform = "scale(1.05)";
            e.currentTarget.style.boxShadow = "0 6px 16px rgba(0,0,0,0.35)";
          }}
          onMouseOut={(e) => {
            e.currentTarget.style.background =
              "linear-gradient(135deg, #667eea, #764ba2)";
            e.currentTarget.style.transform = "scale(1)";
            e.currentTarget.style.boxShadow = "0 4px 12px rgba(0,0,0,0.25)";
          }}
        >
          Volver al menú principal
        </Link>
      </div>


      {/*Cuarta Sección*/}
      <div
        style={{
          flex: 1,
          display: "flex", // dividir en columnas
          justifyContent: "center",
          alignItems: "flex-start",
          gap: "10px",
          padding: "5px",
        }}
      >
      </div>


      {/* Mensaje de Aprobación*/}
      {showMessage && (
        <div
          style={{
            position: "fixed",
            top: 0,
            left: 0,
            width: "100%",
            height: "100%",
            backgroundColor: "rgba(0,0,0,0.6)", // fondo semi-transparente
            display: "flex",
            justifyContent: "center",
            alignItems: "center",
            zIndex: 1000,
          }}
        >
          <div
            style={{
              backgroundColor: "white",
              padding: "30px",
              borderRadius: "15px",
              textAlign: "center",
              boxShadow: "0 5px 20px rgba(0,0,0,0.3)",
              animation: "fadeIn 0.5s ease", // animación opcional
            }}
          >
            <h2 style={{ color: "green" }}>✅ ¡El producto cumple las especificaciones! ✅</h2>
            <button
              onClick={() => {
                setShowMessage(false);   // Cierra el aviso
                setLED1On("disp");        // Apaga LED1 Prueba Cortocircuito
                setLED2On("disp");        // Apaga LED2 Prueba Polaridad       // Limpia casillas
                //  setJSONCorto("");        // Limpia JSON
                //   setJSONPolaridad(""); 
              }}
            >
              Cerrar
            </button>
          </div>
        </div>
      )}


      {/* Mensaje de Error*/}
      {showMessageError && (
        <div
          style={{
            position: "fixed",
            top: 0,
            left: 0,
            width: "100%",
            height: "100%",
            backgroundColor: "rgba(0,0,0,0.6)", // fondo semi-transparente
            display: "flex",
            justifyContent: "center",
            alignItems: "center",
            zIndex: 1000,
          }}
        >
          <div
            style={{
              backgroundColor: "white",
              padding: "30px",
              borderRadius: "15px",
              textAlign: "center",
              boxShadow: "0 5px 20px rgba(0,0,0,0.3)",
              animation: "fadeIn 0.5s ease", // animación opcional
            }}
          >
            <h2 style={{ color: "red" }}>❌ ¡El producto presenta fallas! ❌</h2>
            <button
              onClick={() => {
                setShowMessageError(false);   // Cierra el aviso
              }}
            >
              Cerrar
            </button>
          </div>
        </div>
      )}



      <div style={{ position: "fixed", bottom: 0, left: 0, width: "100%", height: "60px", zIndex: 100 }}>
        <Footer />
      </div>



    </main>
  );
}

