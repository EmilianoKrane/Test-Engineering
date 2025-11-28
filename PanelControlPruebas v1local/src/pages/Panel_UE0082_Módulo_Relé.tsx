

import React, { useState, useEffect, useRef } from "react";
import Header from "../components/Header";
import Footer from "../components/Footer";
import { Link } from 'react-router-dom';
import "../components/RelayVisual.css";

export default function Content() {

  const [stateRelay1, setStateRelay1] = useState("NO");
  const [stateRelay2, setStateRelay2] = useState("NC");

  // Estados para LEDS
  const [ledStatesR1, setLedStatesR1] = useState<string[]>(["disp", "disp"]);
  const ledLabelsR1 = ["Inicio", "Fin"];

  const [ledStatesR2, setLedStatesR2] = useState<string[]>(["disp", "disp"]);
  const ledLabelsR2 = ["Inicio", "Fin"];

  const updateLed = (index: number, newState: "on" | "off" | "waiting") => {
    setLedStatesR1((prev) => {
      const updated = [...prev]; // copiamos el array
      updated[index] = newState; // cambiamos el estado del LED específico
      return updated;            // devolvemos el array actualizado
    });
  };

  const updateLed33 = (index: number, newState: "on" | "off" | "waiting") => {
    setLedStatesR2((prev) => {
      const updated = [...prev]; // copiamos el array
      updated[index] = newState; // cambiamos el estado del LED específico
      return updated;            // devolvemos el array actualizado
    });
  };


  // Estado final del producto
  const [stateProducto, setProducto] = useState("disp");



  // Estado de Observaciones
  const [observaciones, setObservaciones] = useState("");


  //const [voltajeMes, setVoltajeMes] = useState("");
  //const [corrienteMes, setCorrienteMes] = useState("");
  //const [potenciaMes, setPotenciaMes] = useState("");

  const vRef = useRef("");
  const cRef = useRef("");
  const pRef = useRef("");




  // Estado para el contenido de la caja de texto
  const [ID_Producto, setMessages] = useState<string>("");
  //const [idTestBench, setIdTestBench] = useState<number | null>(null);

  let nuevoIDTestBench: number | null = null;

  // Referencia al input 
  const inputRef = useRef<HTMLInputElement>(null);

  // Estado para la comunicación serie por puerto
  const [port1, setPort1] = useState<SerialPort | null>(null);
  const [port2, setPort2] = useState<SerialPort | null>(null);


  // Estado para el aviso de finalizado
  const [showMessage, setShowMessage] = useState(false);
  const [showMessageError, setShowMessageError] = useState(false);

  // Estado para el botón de Check
  //const [botonPresionado, setBotonPresionado] = useState(false);

  // Función que devuelve una promesa y espera a que el usuario presione el botón
  /*
  const esperarBoton = (): Promise<void> => {
    return new Promise((resolve) => {
      const handleClick = () => {
        resolve();                 // resuelve la promesa
        console.log("El botón ha sido presionadoooo");
        window.removeEventListener("clickCheck", handleClick as any);
      };
  
      window.addEventListener("clickCheck", handleClick as any);
    });
  };
  */

  // Mensaje de producto aprobado
  useEffect(() => {
    if (stateProducto === "Valido") {
      setShowMessage(true);
      setShowMessageError(false);  // asegurarte de ocultar el mensaje de error
    } else if (stateProducto === "Defectuoso") {
      setShowMessage(false);
      setShowMessageError(true);
    } else {
      setShowMessage(false);
      setShowMessageError(false);
    }
  }, [stateProducto]);


  // Cuando el componente carga, ponemos el foco en el input
  useEffect(() => {
    inputRef.current?.focus();
  }, []);

  // Estado para el mensaje temporal
  const [tempMessage1, setTempMessage1] = useState<string | null>(null);
  const [tempMessage2, setTempMessage2] = useState<string | null>(null);


  // Función para mostrar mensaje temporal
  const showTemporaryMessage = (msg: string) => {
    setTempMessage1(msg);           // Mostrar mensaje
    setTimeout(() => {
      setTempMessage1(null);        // Ocultar mensaje después de 8 segundos
    }, 10000);
  };

  /**
   * Muestra un mensaje temporal hasta que la medición se complete o expire el timeout
   * @param msg Mensaje a mostrar
   * @param checkFunc Función que valida la medición (retorna boolean o Promise<boolean>)
   * @param timeout Tiempo máximo de espera en ms (default 10000ms = 10s)
   * @returns Promise<boolean> → true si medición correcta, false si timeout
   */

  const showMessageUntilSuccess = async (
    msg: string,
    checkFunc: () => boolean | Promise<boolean>,
    timeout = 60000
  ): Promise<boolean> => {
    setTempMessage2(msg); // Mostrar mensaje

    try {
      const result = await Promise.race([
        checkFunc(), // Ejecuta la medición una sola vez
        new Promise<boolean>((resolve) =>
          setTimeout(() => resolve(false), timeout) // Timeout
        )
      ]);

      setTempMessage2(null); // Ocultar mensaje
      return result; // true si medición correcta, false si timeout
    } catch (err) {
      console.error("❌ No se actualizó la posición del DipSwitch:", err);
      setTempMessage2(null);
      return false;
    }
  };




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

      setMessages(e.currentTarget.value);

      // Esperar 30 segundos antes de borrar el input
      setTimeout(() => {
        setMessages("");
      }, 40000); // 10000ms = 10 segundos

    }
  };




  const initValores = () => {

    updateLed(0, "waiting"); // LEDS indicadores en espera
    updateLed(1, "waiting");
    updateLed(2, "waiting");
    updateLed(3, "waiting");
    updateLed(4, "waiting");


    updateLed33(0, "waiting"); // LEDS de 3.3V indicadores en espera
    updateLed33(1, "waiting");
    updateLed33(2, "waiting");
    updateLed33(3, "waiting");
    updateLed33(4, "waiting");
  }

  const connectPULSAR = async () => {
    try {
      const selectedPort: SerialPort = await (navigator as any).serial.requestPort();
      await selectedPort.open({ baudRate: 115200 });
      setPort1(selectedPort);
      console.log("Puerto COM Test conectado ");
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

      // empezamos a leer de Arduino
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
      console.log("JSON enviado a PULSAR:", char);
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


  // Función para evaluar cada una de las mediciones hechas
  const EvaluarPrueba = async (
    V: string, I: string, P: string, setpoint: number, LED: number, tolerancia: number) => {

    const volt = Number(V)
    const voltaje = Number(volt.toFixed(3))
    let statusPrueba = 2;  // Estatus de no aprobado
    const error = Math.abs(setpoint - voltaje) / setpoint * 100;

    if (error < tolerancia) { // En porcentaje
      statusPrueba = 1;
      updateLed(LED, "on");
    }
    else {
      updateLed(LED, "off");
      console.log("❌ Prueba fallida");
    }

    let id_prueba = 8; // ID por default
    switch (setpoint) {
      case 3.3:
        id_prueba = 19;
        break;
      case 5:
        id_prueba = 20;
        break;
      case 9:
        id_prueba = 21;
        break;
      case 12:
        id_prueba = 22;
        break;
      case 15:
        id_prueba = 23;
        break;
      case 20:
        id_prueba = 24;
        break;
    }


    console.log("El estatus de prueba es: ", statusPrueba);
    console.log("Voltaje medido:", voltaje);
    console.log("Setpoint: ", setpoint);
    console.log("Error entre mediciones: ", error);

    const JSON_Test = {
      "id_setPruebas": nuevoIDTestBench,
      "id_tipo_prueba": id_prueba,
      "id_status_prueba": statusPrueba,  // 1: Prueba OK  2: Prueba Falló
      "comentarios": observaciones,
      "parametro_1": "V: " + V,
      "parametro_2": "I: " + I,
      "parametro_3": "P: " + P
    };


    const exito_Test = await enviarNewTest(JSON_Test);
    console.log("Enviando JSON a BD:", JSON.stringify(JSON_Test));
    //const exito_Test = true;

    if (exito_Test) {
      console.log("✅ Datos de Test enviados correctamente");
      return true;
    } else {
      alert("❌ Falló el envío del test a la Base de Datos, favor de repetir prueba.");
      return false;
    }
  }




  // Función de verificación de medición asíncrona
  const verificarMedicion = async (): Promise<boolean> => {
    try {
      // Lectura inicial
      await delay(1200);

      await delay(500); // Esperamos a que se actualice el ref
      const v_init = Number(vRef.current?.trim() ?? "");
      console.log("V inicial: ", v_init);

      const maxIntentos = 18;  // máximo 18 intentos
      const intervalo = 2000;  // 2 segundo entre lecturas

      for (let t = 0; t < maxIntentos; t++) {

        await delay(intervalo);

        let v_fin = Number(vRef.current?.trim() ?? "");
        console.log(`Intento ${t + 1}: Lectura actual = ${v_fin}, Inicial = ${v_init}`);

        let deltaV = v_fin - v_init;

        if (deltaV > 2) {
          return true; // La medición aumentó, es válida
        }
      }

      return false; // No aumentó después de todos los intentos
    } catch (err) {
      console.error("❌ Error en la medición:", err);
      return false;
    }
  };

  // Función de verificación de medición asíncrona
  const Medir33V = async (): Promise<boolean> => {
    try {
      let statusPrueba = 2;
      // Switch en relevador para medir 3.3V
      await sendToPulsar('{"Function":"Lectura V33"}');
      console.log("➡️ Configurando la carga variable a 2A...");

      await delay(1200);

      console.log("➡️ Encendiendo la carga variable...");

      await delay(2000);

      // Lectura inicial

      await delay(1200); // Esperamos a que se actualice el ref
      const v_med = Number(vRef.current?.trim() ?? "");
      let c = cRef.current?.trim() ?? "";
      let p = pRef.current?.trim() ?? "";

      console.log("V medido para 3.3V: ", v_med);
      if (v_med > 2.8 && v_med < 3.5) {
        statusPrueba = 1;
      }

      console.log("➡️ Apagando la carga variable...");

      await delay(1200);
      sendToPulsar('{"Function":"Lectura Vusb"}');

      const JSON_Test = {
        "id_setPruebas": nuevoIDTestBench,
        "id_tipo_prueba": 19,
        "id_status_prueba": statusPrueba,  // 1: Prueba OK  2: Prueba Falló
        "comentarios": observaciones,
        "parametro_1": "V: " + String(v_med),
        "parametro_2": "I: " + c,
        "parametro_3": "P: " + p
      };

      await enviarNewTest(JSON_Test);

      if (statusPrueba == 1) return true;
      else return false;

    } catch (err) {
      console.error("❌ Error en la medición:", err);
      return false;
    }
  };


  // --- Secuencia de Ejecución ---
  const Arranque = async () => {
    const start = Date.now(); // tiempo inicial

    setStateRelay1("ON");
    setStateRelay2("ON");

    // Verificación de puertos de comunicación 
    if (!port1) {
      alert("Selecciona un puerto COM para el controlador.");
      return;
    }


    // Rutina de ejecución
    try {

      //Valores iniciales 
      initValores();

      const exito = await enviarIDProducto(String(nuevoIDTestBench));
      //const exito = true; 
      if (!exito) return; // si falla, salimos de la secuencia



      // ===== Rutina de Mediciones de Voltaje =====

      // == PROCESO PARA 5V
      sendToPulsar('{"Function":"Lectura Vusb"}');
      showTemporaryMessage(" ➡️ Coloca el DipSwitch como indica la figura para 5V");
      console.log("➡️ Configurando la carga variable a 3A...");

      await delay(10000);

      //await esperarBoton(); // Espera botón de ¡Listo!
      console.log("➡️ Encendiendo la carga variable...");

      await delay(2000);

      console.log("➡️ Leyendo valores de la carga variable...");


      await delay(1200);
      let v = vRef.current?.trim() ?? "";
      let c = cRef.current?.trim() ?? "";
      let p = pRef.current?.trim() ?? "";

      console.log("➡️ El valor de voltaje es: ", v);
      console.log("➡️ El valor de corriente es: ", c);
      console.log("➡️ El valor de potencia es: ", p);

      console.log("➡️ Evaluando prueba y envío de test a BD...")
      let status_envio = EvaluarPrueba(v, c, p, 5, 0, 20); // V, I, P, SetPoint, No.LED, %Error
      if (!status_envio) return; // si falla, salimos de la secuencia

      console.log("➡️ Apagando la carga variable...");

      await delay(1200);

      // Medición de 3.3V
      let ExitoLED33 = await Medir33V();
      if (ExitoLED33) {
        updateLed33(0, "on");
      } else {
        updateLed33(0, "off");
      }



      // == PROCESO PARA 9V

      let DipSwitch = await showMessageUntilSuccess(
        " ➡️ Coloca el DipSwitch como indica la figura para 9V",
        verificarMedicion, 60000); // Tiempo máximo 1 min

      if (DipSwitch) {
        console.log("✅ Se cambió la posición del DipSwitch");
      } else {
        alert("❌ No se cambió la posición del DipSwitch. Reinicia la prueba");
        return;
      }

      console.log("➡️ Configurando la carga variable a 3A...");

      await delay(1200);
      console.log("➡️ Encendiendo la carga variable...");

      await delay(2000);

      console.log("➡️ Leyendo valores de la carga variable...");

      await delay(1200);
      v = vRef.current?.trim() ?? "";
      c = cRef.current?.trim() ?? "";
      p = pRef.current?.trim() ?? "";

      console.log("➡️ El valor de voltaje es: ", v);
      console.log("➡️ El valor de corriente es: ", c);
      console.log("➡️ El valor de potencia es: ", p);

      await delay(2000);
      console.log("➡️ Apagando la carga variable...");


      console.log("➡️ Evaluando prueba y envío de test a BD...")
      status_envio = EvaluarPrueba(v, c, p, 9, 1, 20); // V, I, P, SetPoint, No.LED, %Error
      if (!status_envio) return; // si falla, salimos de la secuencia

      // Medición de 3.3V
      ExitoLED33 = await Medir33V();
      if (ExitoLED33) {
        updateLed33(1, "on");
      } else {
        updateLed33(1, "off");
      }


      // == PROCESO PARA 12V

      DipSwitch = await showMessageUntilSuccess(
        " ➡️ Coloca el DipSwitch como indica la figura para 12V",
        verificarMedicion, 60000); // Tiempo máximo 1 min

      if (DipSwitch) {
        console.log("✅ Se cambió la posición del DipSwitch");
      } else {
        alert("❌ No se cambió la posición del DipSwitch. Reinicia la prueba");
        return;
      }

      console.log("➡️ Configurando la carga variable a 3A...");

      await delay(1200);
      console.log("➡️ Encendiendo la carga variable...");

      await delay(2000);

      console.log("➡️ Leyendo valores de la carga variable...");

      await delay(1200);
      v = vRef.current?.trim() ?? "";
      c = cRef.current?.trim() ?? "";
      p = pRef.current?.trim() ?? "";

      console.log("➡️ El valor de voltaje es: ", v);
      console.log("➡️ El valor de corriente es: ", c);
      console.log("➡️ El valor de potencia es: ", p);

      await delay(2000);
      console.log("➡️ Apagando la carga variable...");


      console.log("➡️ Evaluando prueba y envío de test a BD...")
      status_envio = EvaluarPrueba(v, c, p, 12, 2, 20); // V, I, P, SetPoint, No.LED, %Error
      if (!status_envio) return; // si falla, salimos de la secuencia

      // Medición de 3.3V
      ExitoLED33 = await Medir33V();
      if (ExitoLED33) {
        updateLed33(2, "on");
      } else {
        updateLed33(2, "off");
      }


      // == PROCESO PARA 15V

      DipSwitch = await showMessageUntilSuccess(
        " ➡️ Coloca el DipSwitch como indica la figura para 15V",
        verificarMedicion, 60000); // Tiempo máximo 1 min

      if (DipSwitch) {
        console.log("✅ Se cambió la posición del DipSwitch");
      } else {
        alert("❌ No se cambió la posición del DipSwitch. Reinicia la prueba");
        return;
      }

      console.log("➡️ Configurando la carga variable a 3A...");

      await delay(1200);
      console.log("➡️ Encendiendo la carga variable...");

      await delay(2000);

      console.log("➡️ Leyendo valores de la carga variable...");

      await delay(1200);
      v = vRef.current?.trim() ?? "";
      c = cRef.current?.trim() ?? "";
      p = pRef.current?.trim() ?? "";

      console.log("➡️ El valor de voltaje es: ", v);
      console.log("➡️ El valor de corriente es: ", c);
      console.log("➡️ El valor de potencia es: ", p);

      await delay(2000);
      console.log("➡️ Apagando la carga variable...");


      console.log("➡️ Evaluando prueba y envío de test a BD...")
      status_envio = EvaluarPrueba(v, c, p, 15, 3, 20); // V, I, P, SetPoint, No.LED, %Error
      if (!status_envio) return; // si falla, salimos de la secuencia

      // Medición de 3.3V
      ExitoLED33 = await Medir33V();
      if (ExitoLED33) {
        updateLed33(3, "on");
      } else {
        updateLed33(3, "off");
      }


      // == PROCESO PARA 20V

      DipSwitch = await showMessageUntilSuccess(
        " ➡️ Coloca el DipSwitch como indica la figura para 20V",
        verificarMedicion, 60000); // Tiempo máximo 1 min

      if (DipSwitch) {
        console.log("✅ Se cambió la posición del DipSwitch");
      } else {
        alert("❌ No se cambió la posición del DipSwitch. Reinicia la prueba");
        return;
      }

      console.log("➡️ Configurando la carga variable a 5A...");

      await delay(1200);
      console.log("➡️ Encendiendo la carga variable...");

      await delay(2000);

      console.log("➡️ Leyendo valores de la carga variable...");


      await delay(1200);
      v = vRef.current?.trim() ?? "";
      c = cRef.current?.trim() ?? "";
      p = pRef.current?.trim() ?? "";

      console.log("➡️ El valor de voltaje es: ", v);
      console.log("➡️ El valor de corriente es: ", c);
      console.log("➡️ El valor de potencia es: ", p);

      await delay(2000);
      console.log("➡️ Apagando la carga variable...");

      console.log("➡️ Evaluando prueba y envío de test a BD...")
      status_envio = EvaluarPrueba(v, c, p, 20, 4, 20); // V, I, P, SetPoint, No.LED, %Error
      if (!status_envio) return; // si falla, salimos de la secuencia

      // Medición de 3.3V
      ExitoLED33 = await Medir33V();
      if (ExitoLED33) {
        updateLed33(4, "on");
      } else {
        updateLed33(4, "off");
      }

      if (ledStatesR2.every(state => state === "on") &&
        ledStatesR2.every(state => state === "on")) {
        setProducto("Valido");
      }
      else {
        setProducto("Defectuoso");
      }


      console.log("✅ Secuencia completada correctamente");

      const end = Date.now(); // tiempo final
      const elapsed = (end - start) / 1000; // en segundos

      console.log(`✅ Finalizado en ${elapsed.toFixed(2)} segundos`);

    } catch (err) {
      console.error("❌ Error en la secuencia:", err);
    }
  };




  const enviarIDProducto = async (ID_Producto: string) => {
    const JSON_ID = {
      id_proyecto: 103,     // ID de Proyecto fijo
      id_mac: "",
      uid: "",
      id_numero_serie: ID_Producto,
      id_tecnico: 1,
      comentarios_generales: "Pruebas Fuente para Protoboard",
    };

    const resultado = await enviarNewTestBench(JSON_ID);
    console.log(resultado);

    await delay(2000);

    if (resultado.success && resultado.id_testBench) {
      console.log("✅ Datos de ID enviados correctamente");


      nuevoIDTestBench = resultado.id_testBench;
      console.log(nuevoIDTestBench);


      //setIdTestBench(nuevoIDTestBench); 

      return true; // aquí ya sabes que sí tienes el ID
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
  const enviarNewTest = async (jsonData: Record<string, any>): Promise<boolean> => {
    try {
      const response = await fetch("/api/BoardTesting/setNewTest", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(jsonData),
      });

      await delay(1000);
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
          gap: "15px",           // espacio entre elementos
          marginTop: "180px"
        }}
      >
        <h2 style={{ margin: 0 }}>ID del producto:</h2>

        <input
          ref={inputRef}
          type="text"
          value={ID_Producto}
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



      {/* Sección Principal */}
      <div
        style={{
          display: "flex",
          flexDirection: "row", // fila principal
          justifyContent: "space-between",
          alignItems: "flex-start",
          border: "2px solid #333",
          borderRadius: "10px",
          padding: "20px 30px",
          backgroundColor: "#f5f5f5",
          boxShadow: "2px 2px 10px rgba(0,0,0,0.2)",
          width: "80%",
          maxWidth: "500px",
          margin: "20px auto",
          gap: "0px",
        }}
      >
        {/* --- Columna Izquierda: Botón --- */}
        <div
          style={{
            display: "flex",
            justifyContent: "center",
            alignItems: "center",
            marginTop: "90px",
            flex: 1,
          }}
        >
          <button
            onClick={Arranque}
            style={{
              padding: "20px 0px",
              borderRadius: "10px",
              border: "none",
              backgroundColor: "#3a26efff",
              color: "white",
              fontWeight: "bold",
              cursor: "pointer",
              boxShadow: "0 3px 6px rgba(0,0,0,0.3)",
              transition: "background-color 0.3s ease",
              height: "80px",
              width: "110px",
              fontSize: "24px",
            }}
            onMouseOver={(e) =>
              (e.currentTarget.style.backgroundColor = "#11067aff")
            }
            onMouseOut={(e) =>
              (e.currentTarget.style.backgroundColor = "#3a26efff")
            }
          >
            ¡Inicio!
          </button>
        </div>


        {/* --- Columna Derecha: Relevadores con LEDs debajo --- */}
        <div
          style={{
            display: "flex",
            flexDirection: "row", // relevador 1 y relevador 2 lado a lado
            justifyContent: "center",
            gap: "40px",
            flex: 3,
          }}
        >
          {/* Relevador 1 + LEDs */}
          <div
            style={{
              display: "flex",
              flexDirection: "column",
              alignItems: "center",
              gap: "15px",
            }}
          >
            <div className="relay-box">
              <div className="relay-label">Relevador 1</div>
              <div className="relay-frame">
                <div
                  className={`contact nc ${stateRelay1 === "NC" ? "active-nc" : ""
                    }`}
                >
                  NC
                </div>
                <div className={`arm ${stateRelay1}`}></div>
                <div
                  className={`contact no ${stateRelay1 === "NO" ? "active-no" : ""
                    }`}
                >
                  NO
                </div>
              </div>
            </div>

            {/* LEDs debajo de Relevador 1 */}
            <div
              style={{
                display: "flex",
                flexDirection: "row",
                gap: "15px",
              }}
            >
              {ledStatesR1.map((state, index) => {
                const isOn = state === "on";
                const isOff = state === "off";
                const isWaiting = state === "waiting";

                return (
                  <div
                    key={index}
                    style={{
                      display: "flex",
                      flexDirection: "column",
                      alignItems: "center",
                    }}
                  >
                    <div
                      style={{
                        width: "30px",
                        height: "30px",
                        borderRadius: "50%",
                        border: "1px solid black",
                        backgroundColor: isOn
                          ? "#4CAF50"
                          : isOff
                            ? "#F44336"
                            : isWaiting
                              ? "#FFC107"
                              : "#e6e6e6ff",
                        boxShadow:
                          "0 6px 6px rgba(0,0,0,0.3), inset 0 2px 3px rgba(255,255,255,0.3)",
                        transition: "all 0.3s ease",
                      }}
                    />
                    <div
                      style={{
                        marginTop: "6px",
                        fontSize: "12px",
                        fontWeight: "bold",
                        color: "#333",
                        textAlign: "center",
                      }}
                    >
                      {ledLabelsR1[index]}
                    </div>
                  </div>
                );
              })}
            </div>
          </div>

          {/* Relevador 2 + LEDs */}
          <div
            style={{
              display: "flex",
              flexDirection: "column",
              alignItems: "center",
              gap: "15px",
            }}
          >
            <div className="relay-box">
              <div className="relay-label">Relevador 2</div>
              <div className="relay-frame">
                <div
                  className={`contact nc ${stateRelay2 === "NC" ? "active-nc" : ""
                    }`}
                >
                  NC
                </div>
                <div className={`arm ${stateRelay2}`}></div>
                <div
                  className={`contact no ${stateRelay2 === "NO" ? "active-no" : ""
                    }`}
                >
                  NO
                </div>
              </div>
            </div>

            {/* LEDs debajo de Relevador 2 */}
            <div
              style={{
                display: "flex",
                flexDirection: "row",
                gap: "15px",
              }}
            >
              {ledStatesR2.map((state, index) => {
                const isOn = state === "on";
                const isOff = state === "off";
                const isWaiting = state === "waiting";

                return (
                  <div
                    key={index}
                    style={{
                      display: "flex",
                      flexDirection: "column",
                      alignItems: "center",
                    }}
                  >
                    <div
                      style={{
                        width: "30px",
                        height: "30px",
                        borderRadius: "50%",
                        border: "1px solid black",
                        backgroundColor: isOn
                          ? "#4CAF50"
                          : isOff
                            ? "#F44336"
                            : isWaiting
                              ? "#FFC107"
                              : "#e6e6e6ff",
                        boxShadow:
                          "0 6px 6px rgba(0,0,0,0.3), inset 0 2px 3px rgba(255,255,255,0.3)",
                        transition: "all 0.3s ease",
                      }}
                    />
                    <div
                      style={{
                        marginTop: "6px",
                        fontSize: "12px",
                        fontWeight: "bold",
                        color: "#333",
                        textAlign: "center",
                      }}
                    >
                      {ledLabelsR2[index]}
                    </div>
                  </div>
                );
              })}
            </div>
          </div>
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
          marginTop: "0px",
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
                setShowMessage(false);      // Cierra el aviso de aprobación

                setMessages("");            // Limpia casillas
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


      {/*Mensaje temporal */}
      {tempMessage1 && (
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
            alignItems: "flex-start", // arriba
            paddingTop: "20px", // separación desde arriba
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
              animation: "fadeIn 0.5s ease", // animación 
            }}
          >
            <h2 style={{ color: "rgba(108, 52, 172, 0.8)" }}>{tempMessage1}</h2>

          </div>
        </div>
      )}

      {/*Mensaje temporal */}
      {tempMessage2 && (
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
            alignItems: "flex-start", // arriba
            paddingTop: "20px", // separación desde arriba
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
              animation: "fadeIn 0.5s ease", // animación 
            }}
          >
            <h2 style={{ color: "rgba(108, 52, 172, 0.8)" }}>{tempMessage2}</h2>

          </div>
        </div>
      )}


      <div style={{ position: "fixed", bottom: 0, left: 0, width: "100%", height: "60px", zIndex: 100 }}>
        <Footer />
      </div>



    </main>
  );
}



