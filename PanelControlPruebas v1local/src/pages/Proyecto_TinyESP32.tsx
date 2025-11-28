import React, { useState, useEffect, useRef } from "react";
import Header from "../components/Header";
import Footer from "../components/Footer";
import { Link } from 'react-router-dom';

export default function Content() {

  // Estados para los LEDs
  const [stateLED1, setLED1On] = useState("disp");
  const [stateLED2, setLED2On] = useState("disp");
  const [stateLED3, setLED3On] = useState("disp");

  // Estado para el contenido de la caja de texto para ID 
  const [IDProducto, setMessages] = useState<string>("");
  let nuevoIDTestBench: number | null = null; 

//const [resultadoMic, setResultadoMic] = useState<number>(2);
//const [resultadoSD, setResultadoSD] = useState<number>(2);
//const [resultadoBtn, setResultadoBtn] = useState<number>(2);

let resultadoMic = 3; 
let resultadoSD = 3; 
let resultadoBtn = 3; 

  // Referencia al input 
  const inputRef = useRef<HTMLInputElement>(null);

  // Estado de Observaciones
  const [observaciones, setObservaciones] = useState("");

  // Estado para la comunicación serie por puerto
  const [port1, setPort1] = useState<SerialPort | null>(null);


  // Estado para el aviso de finalizado
  const [showMessage, setShowMessage] = useState(false);
  const [showMessageError, setShowMessageError] = useState(false);
  const [showMessageEspera, setShowMessageEspera] = useState(false);

  useEffect(() => {
  if (stateLED1 == "on" && stateLED2 == "on" && stateLED3 == "on" ) {
    setShowMessage(true);   // Mostrar el aviso
  }}, [stateLED1, stateLED2, stateLED3]);

  useEffect(() => {
  if (stateLED1 == "off" || stateLED2 == "off" || stateLED3 == "off" ) {
    setShowMessageError(true);   // Mostrar el aviso de error
  }}, [stateLED1, stateLED2, stateLED3]);


  // Cuando el componente carga, ponemos el foco en el input
  useEffect(() => {
    inputRef.current?.focus();
    }, []);


// Función para esperar N milisegundos
const delay = (ms: number) => new Promise(resolve => setTimeout(resolve, ms));


// Manejar cuando el GM65 “escribe” algo
const handleChange = (e: React.ChangeEvent<HTMLInputElement>) => {
  setMessages(e.target.value); // copiar lo que se escribió al input
};

// Manejar cuando el GM65 manda "Enter" después del código
const handleKeyDown = (e: React.KeyboardEvent<HTMLInputElement>) => {
if (e.key === "Enter") {
  console.log("Código leído:", e.currentTarget.value);

    // Mostrar en el input y limpiar valor
    //e.currentTarget.value = "";
    setMessages(e.currentTarget.value);

    // Esperar 3 segundos antes de borrar el input
    setTimeout(() => {
      setMessages(""); 
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

const getColor3 = () => {
  switch (stateLED3) {
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




const connectPULSAR = async () => {
  try {
    // pedir puerto al usuario
    const selectedPort: SerialPort = await (navigator as any).serial.requestPort();
    await selectedPort.open({ baudRate: 921600 }); 

    // guardamos el puerto en el estado
    setPort1(selectedPort);
    console.log("Puerto COM Test conectado ");

    // empezamos a leer de Arduino
    readFromPULSAR(selectedPort);
  } catch (err) {
    console.error("Error al conectar con la placa:", err);
  }
};



const sendToPulsar = async (char: string) => {
 if (port1 && port1.writable){
    const encoder = new TextEncoder();
    const writer = port1.writable.getWriter();
    await writer.write(encoder.encode(char));
    writer.releaseLock();
    console.log("JSON enviado a PULSAR:", char); // <-- ahora sí imprime el dato enviado
 }
};



const readFromPULSAR = async (selectedPort: SerialPort) => {
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
      
      for (const part of parts){

        const trimmed = part.trim();
        if (!trimmed) continue; //evita parsear vacío

        try {
          const parsed = JSON.parse(trimmed);
          console.log("JSON recibido:", parsed);

          // 🚫 Ignorar si cualquier valor es "waiting"
          if (Object.values(parsed).includes("waiting")) {
            console.log("⏳ Esperando resultado real, ignorando este mensaje...");
            setShowMessageEspera(true);
            continue; 
          }

          // Emitimos evento global si quieres usarlo en otro lado
            setShowMessageEspera(false);
            window.dispatchEvent(new CustomEvent("jsonReceived", { detail: parsed }));

          // enviarNewTest(statusPrueba, idPrueba)
          // statusPrueba: 1 -> OK | 2 -> Falla


          // 🔹 Tomar decisiones según claves
          if (parsed.mic === "good") {
            console.log("Micrófono OK ✅");
            setLED1On("on");
            resultadoMic = 1; 
          } else {
            console.log("Micrófono Falló ❌");
            setLED1On("off");
          }

          if (parsed.sd === "good") {
            console.log("SD OK ✅");
            resultadoSD = 1; 
            setLED2On("on");
          } else {
            console.log("SD Falló ❌");
            setLED2On("off");
          }

          if (parsed.button === "good") {
            console.log("Botón OK ✅");
            resultadoBtn = 1; 
            setLED3On("on");
          }
          else {
            console.log("Botón Falló ❌");
            setLED3On("off");
          }

          if (parsed.overall === "good") {
            console.log("Prueba general OK ✅");
          } else {
            console.log("Prueba general Falló ❌");
          }


        } catch (err) {
          console.error("Error parseando JSON:", err, trimmed);
        }
      }
    }
  }

};


// Función de envío de ID a la Base de Datos
const enviarIDProducto = async (ID_Producto: string) => {
  const JSON_ID = {
    id_proyecto: 118,     // ID de Proyecto TEST
    id_mac: "",
    uid: "", 
    id_numero_serie: ID_Producto,
    id_tecnico: 1,
    comentarios_generales: "",
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

    console.log(JSON_Test)

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


const esperarResultado = () =>
  new Promise<void>((resolve) => {
    const handler = (e: CustomEvent) => {
      const parsed = e.detail;
      // Cuando llega el resultado final (no waiting)
      if (!Object.values(parsed).includes("waiting")) {
        resultadoMic = parsed.mic === "good" ? 1 : 2;
        resultadoSD  = parsed.sd  === "good" ? 1 : 2;
        resultadoBtn = parsed.button === "good" ? 1 : 2;
        window.removeEventListener("jsonReceived", handler as any);
        resolve();
      }
    };
    window.addEventListener("jsonReceived", handler as any);
  });



// --- Secuencia completa ---
const Arranque = async () => {

if (!port1) {
    alert("Selecciona un puerto COM para el controlador.");
    return;
  }

  setLED1On("waiting");
  setLED2On("waiting");
  setLED3On("waiting");

  // Rutina de ejecución
  try {

        const exito = await enviarIDProducto(String(IDProducto));
        if (!exito) return; 

        // ===== Rutina de Prueba =====
        await sendToPulsar(JSON.stringify({ test: "all" }) + "\n");
        
        await esperarResultado();

        await enviarNewTest(resultadoMic, 26);
        await enviarNewTest(resultadoSD, 11);
        await enviarNewTest(resultadoBtn, 12);



  } catch (err) {
    console.error("❌ Error en la secuencia:", err);
  }
};







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

    <div style={{ position: "fixed", top: 0, left: 0, width: "100%", height: "60px", zIndex: 100  }}>
      <Header />
    </div>


    {/* Primera Sección: ID de producto */}
    <div
        style={{
            display: "flex",       // alineación en fila
            alignItems: "center",  // centrado vertical
            gap: "10px",           // espacio entre elementos
            marginTop: "190px",
        }}
        >
        <h2 style={{ margin:  0}}>ID del módulo Tiny ESP32</h2>

        <input
            ref={inputRef}
            type= "text"
            value={IDProducto}
            onChange={handleChange}
            onKeyDown={handleKeyDown}
            style={{ width: "120px", height: "25px" }}
        />

        {/*COM para Pulsar Control*/}
        <button
        onClick={connectPULSAR}
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
        marginTop: 20 }}

    >Tablero de indicadores 🔎 </h3>

    {/* Configuración Fila 1 */}
    <div style={{ 
        display: "flex", 
        alignItems: "center", 
        gap: "12px" }}>

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
        gap: "12px" }}>
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
        gap: "12px" }}>
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
    style={{ margin:  2}}
    >Microfono I2C</h3>

    <h3
    style={{ margin:  10}}
    >Micro SD</h3>

    <h3
    style={{ margin:  10}}
    >Botón</h3>


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

    {/* LED Carga Variable */}
    <div
    style={{
        width: "30px",
        height: "30px",
        borderRadius: "50%",
        backgroundColor: getColor3(),
        border: "2px solid #333",
        marginBottom: "10px",
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
            marginTop: "80px",
            gap: "10px" }}>

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
                marginRight: "50px",
                marginTop: "30px",
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
        alignItems: "flex-start",   // Alinea label con la parte superior del textarea
        gap: "10px",
        width: "100%",
        maxWidth: "500px",
        margin: "20px auto",        // Centra horizontal y separa de otras secciones
        padding: "10px",
        border: "1px solid #ccc",
        borderRadius: "8px",
        marginBottom: "10px"
      }}
    >
      <label htmlFor="observaciones" style={{ width: "120px", fontWeight: "bold" }}>
        Observaciones:
      </label>
      <textarea
        id="observaciones"
        value={observaciones}          // Necesitas crear un estado para esto
        onChange={(e) => setObservaciones(e.target.value)}
        placeholder="Escribe tus observaciones aquí..."
        style={{
          flex: 1,
          minHeight: "10px",
          padding: "5px",
          resize: "vertical",
          fontSize: "1rem",
        }}
      />
    </div>

    <div
      style={{
        display: "flex",
        justifyContent: "center",
        alignItems: "center",
        //height: "100vh", // ocupa toda la altura de la ventana
        marginTop: "5px",
        marginBottom: "20px",
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
            setLED2On("disp");        // Apaga LED2 Prueba Polaridad
            setLED3On("disp");        // Apaga LED3 Carga Variable
            setMessages("");         // Limpia casillas
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
            setMessages("");         // Limpia casillas
        }}
        >
        Cerrar
        </button>
        </div>
    </div>
    )}


    {/* Mensaje de Espera*/}
    {showMessageEspera && (
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
        <h2 style={{ color: "black" }}> ⚠️ ¡Presiona el botón de la tarjeta! ⚠️</h2>
        <button
        onClick={() => {
            setShowMessageEspera(false);   // Cierra el aviso
            setMessages("");         // Limpia casillas
        }}
        >
        Cerrar
        </button>
        </div>
    </div>
    )}




    <div style={{ position: "fixed", bottom: 0, left: 0, width: "100%", height: "60px", zIndex: 100}}>
      <Footer />
    </div>

    </main>
  );
}
