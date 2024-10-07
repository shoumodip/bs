function tabClick(e) {
    // Do nothing if already active
    if (e.classList.contains("active")) {
        return
    }

    // Switch the active tab
    {
        const [a, b] = e.parentNode.children
        a.classList.toggle("active")
        b.classList.toggle("active")
    }

    // Switch the active code
    {
        const [, , a, b] = e.parentNode.parentNode.children[1].children
        a.classList.toggle("active")
        b.classList.toggle("active")
    }
}

function copyClick(e, ok, text) {
    if (navigator.clipboard) {
        navigator.clipboard.writeText(text)
    } else {
        const textarea = document.createElement("textarea")
        textarea.setAttribute("style", "width: 1px; border:0; opacity:0;")
        document.body.appendChild(textarea)
        textarea.value = text
        textarea.select()
        document.execCommand("copy")
        document.body.removeChild(textarea)
    }

    const toggle = () => {
        e.classList.toggle("hidden")
        ok.classList.toggle("hidden")
    }

    toggle()
    setTimeout(toggle, 1000)
}

function copyClickCode(e) {
    const [, ok, a, b] = e.parentNode.children
    copyClick(e, ok, a.classList.contains("active") ? a.innerText : b.innerText)
}

function copyClickShell(e) {
    const [, ok, a] = e.parentNode.children
    copyClick(e, ok, a.innerText)
}

window.onload = () => {
    const sidebar = document.getElementById("sidebar")
    const toggleSidebar = document.getElementById("toggle-sidebar")
    if (sidebar) {
        const header = document.getElementById("header")

        toggleSidebar.onclick = () => {
            sidebar.classList.toggle("collapsed")
        }

        document.onclick = (event) => {
            if (!sidebar.contains(event.target) && !toggleSidebar.contains(event.target)) {
                sidebar.classList.add("collapsed")
            }
        }

        const padding = parseFloat(window.getComputedStyle(toggleSidebar).left) + header.offsetHeight

        const links = document.querySelectorAll("#sidebar ul li a")
        links.forEach(link => {
            link.addEventListener("click", (e) => {
                e.preventDefault()
                const targetId = link.getAttribute("href")
                const targetSection = document.querySelector(targetId)

                window.scrollTo({
                    top: window.scrollY + targetSection.getBoundingClientRect().top - padding,
                    behavior: "smooth"
                })

                if (window.innerWidth <= 768) {
                    sidebar.classList.add("collapsed")
                }
            })
        })

        if (window.innerWidth > 768) {
            sidebar.classList.remove("collapsed")
        }
    }
}
