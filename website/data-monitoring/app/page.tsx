/* eslint-disable @typescript-eslint/no-explicit-any */
"use client";
import { Toaster, toast } from "react-hot-toast";
import { initializeApp } from "firebase/app";
import {
  getFirestore,
  doc,
  onSnapshot,
  collection,
  query,
} from "firebase/firestore";
import { firebaseConfig } from "./firebase.config";
import { useEffect, useState } from "react";

type Document = {
  Date: Date;
  Message: string;
  Id: string;
  Style: string;
};
export default function Home() {
  const app = initializeApp(firebaseConfig);
  const db = getFirestore(app);

  const [alertMessages, setAlertMessages] = useState<Document[]>([]);

  function getStyle(msg: string) {
    if (
      msg.includes("Unauthorised") ||
      msg.includes("Motion") ||
      msg.includes("Sound") ||
      msg.includes("Temperature")
    ) {
      return "red";
    } else if (msg.includes("Authorised")) {
      return "green";
    } else {
      return "black";
    }
  }

  useEffect(() => {
    const unsub = onSnapshot(collection(db, "alerts"), (snapshot) => {
      const data = snapshot.docs.map((doc) => {
        return {
          Date: new Date(
            doc.data().Date.seconds * 1000 +
              doc.data().Date.nanoseconds / 1000000
          ),
          Message: doc.data().Message,
          Style: getStyle(doc.data().Message),
          Id: doc.id,
        };
      });
      setAlertMessages(data);
      toast.success("Updated alerts");
    });
    return () => unsub();
  }, []);

  function getDivStyle(style: string) {
    let divStyle: string = "border-2 p-2 w-full rounded-md ";
    if (style == "red") {
      divStyle += "border-red-500/40";
      return divStyle;
    } else if (style == "green") {
      divStyle += "border-green-500/40";
      return divStyle;
    } else {
      divStyle += "";
      return divStyle;
    }
  }

  const sortedAlerts = [...alertMessages].sort(
    (firstTime, secondTime) =>
      secondTime.Date.getTime() - firstTime.Date.getTime()
  );

  return (
    <div className="flex flex-col flex-1 items-center justify-center bg-zinc-50 font-sans dark:bg-white">
      <main className="flex flex-1 w-full max-w-3xl flex-col items-center justify-between py-32 px-16 bg-white dark:bg-white sm:items-start">
        <div className="flex flex-col items-center gap-6 text-center sm:items-start sm:text-left">
          <Toaster position="top-center" reverseOrder={false} />
          <h1 className="text-slate-900 font-bold text-4xl">Alerts</h1>
          <div className="scoll-container flex gap-2 flex-col">
            {sortedAlerts.map((item) => (
              <div key={item.Id} className={getDivStyle(item.Style)}>
                <p className="text-slate-900 font-bold">
                  {item.Date.toLocaleString("en-GB", {
                    timeZone: "Europe/London",
                  })}
                </p>
                <p className="text-slate-900">{item.Message}</p>
              </div>
            ))}
          </div>
        </div>
      </main>
    </div>
  );
}
